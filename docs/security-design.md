# Security Design & Reliability Architecture

## 1. Threat Model & Security Philosophy

Operating an HTTP server directly on raw POSIX TCP sockets introduces significant security responsibilities. Because this server is developed in **C17** without high-level language runtime memory isolation, defensive programming is mandatory at every validation boundary.

This document formalizes the threat model, validation limits, defensive safeguards, and failure handling policies.

> **Status Clarification**: The protections detailed herein represent the **planned architectural specification**. They are actively being implemented and verified; until unit, integration, and fuzz testing suites pass completely, the server is not considered production-ready or hardened against external network attacks.

---

## 2. Threat Vector Matrix & Defensive Mitigations

| Threat Vector | Attack Mechanism | Planned Architectural Safeguard |
| :--- | :--- | :--- |
| **Path Traversal** | Requesting paths such as `/../../etc/passwd` or URL-encoded `/%2e%2e/` to read arbitrary host files. | **Strict Canonicalization**: Resolve candidate path using `realpath(3)`. Verify that `public_root` prefix is an exact substring match. Reject any path containing null bytes (`\0`). |
| **Slowloris / Slow Client** | Holding worker threads open indefinitely by transmitting headers one byte at a time. | **Socket Timeouts**: Apply `SO_RCVTIMEO` (default: 5s) and `SO_SNDTIMEO` (default: 5s) via `setsockopt()`. Connections exceeding timeout return `408 Request Timeout` and close. |
| **Memory Exhaustion (OOM)** | Sending gigabytes of continuous headers or unbounded POST request bodies. | **Hard Memory Limits**: Enforce strict compile-time and run-time bounds: max 8 KB total header size, max 64 headers, max 10 MB body size. Drop connections exceeding limits (`413 Payload Too Large`). |
| **Request Smuggling** | Ambiguous message framing (multiple `Content-Length` headers, conflicting transfer encodings). | **Framing Validation**: Strictly reject multiple or conflicting `Content-Length` headers (`400 Bad Request`). Reject chunked transfer-encoding until parser fully validates chunk syntax. |
| **Descriptor Leaks** | Forgetting to close client socket file descriptors on error paths, exhausting OS file table. | **Descriptor Ownership Rule**: Socket descriptors are strictly owned by the processing worker thread. Any error return path explicitly routes through cleanup before returning to the worker loop. |
| **Synchronization Deadlocks** | Contention between connection queue, worker threads, and metrics collection. | **Lock Hierarchy**: Mutexes are acquired in strict hierarchical order: `Queue Lock` -> `Worker Pool Lock` -> `Metrics Lock`. Never acquire locks in reverse order. |

---

## 3. Input Validation Boundaries

```text
+-----------------------+-----------------------------+-----------------------+
| Input Element         | Hard Boundary               | Violation Action      |
+-----------------------+-----------------------------+-----------------------+
| Request-Line Length   | 2,048 bytes                 | 414 URI Too Long      |
| Single Header Length  | 1,024 bytes                 | 400 Bad Request       |
| Total Header Block    | 8,192 bytes (8 KB)          | 431 Headers Too Large |
| Header Count          | 64 maximum headers          | 400 Bad Request       |
| Request Body Size     | 10,485,760 bytes (10 MB)    | 413 Payload Too Large |
| Keep-Alive Requests   | 100 maximum / connection    | Issue Connection: close|
| Keep-Alive Idle Time  | 5 seconds                   | Close Socket Cleanly  |
+-----------------------+-----------------------------+-----------------------+
```

---

## 4. Path Traversal Mitigation Algorithm

When a request arrives for a static resource (e.g. `GET /assets/style.css HTTP/1.1`), the static file module executes the following validation pipeline:

```text
[Raw URI from Client]
        │
        ▼
1. URL Decode & Normalize (e.g., convert %20 to space, reject null bytes '\0')
        │
        ▼
2. Concatenate with Public Root Directory:
   candidate_path = snprintf(public_root, sizeof(buf), "%s/%s", public_root, uri)
        │
        ▼
3. Canonicalize via realpath(candidate_path, resolved_path)
   - If realpath fails (ENOENT) -> Return HTTP 404 Not Found
   - If realpath fails (EACCES) -> Return HTTP 403 Forbidden
        │
        ▼
4. Prefix Verification:
   strncmp(resolved_path, canonical_public_root, strlen(canonical_public_root)) == 0
   - If prefix does NOT match -> Block attempt, log warning, return HTTP 403 Forbidden
        │
        ▼
5. Inspect File Status via stat():
   - If S_ISDIR() -> Check for index.html; if absent, return HTTP 403 (No Directory Listing)
   - If S_ISREG() -> Proceed with secure read / sendfile transmission
```

---

## 5. Resource Ownership & Lifecycle Invariants

1. **Explicit File Descriptor Lifecycle**:
   - The listening thread executes `accept()` and immediately pushes the descriptor into `ConnectionQueue`.
   - Once popped, the assigned worker thread holds single, exclusive ownership of the descriptor.
   - The descriptor is never passed between workers.
   - The worker executes `close()` in all terminating branches (EOF, client timeout, parser error, completion with `Connection: close`).

2. **Clean Memory Management**:
   - Buffers for socket I/O are fixed-size stack or pre-allocated pool buffers.
   - Dynamic memory allocations for POST request bodies are tracked within `HttpRequest` and freed via `http_request_destroy()` before the worker accepts its next connection.

3. **Signal-Safe Graceful Shutdown**:
   - The main process registers `SIGINT` and `SIGTERM` handlers via `sigaction`.
   - Signal handlers set an atomic shutdown flag `server_shutdown_requested = 1` and write a wake byte to a self-pipe or broadcast on condition variables.
   - The server ceases accepting new connections, drains existing queue entries, and joins all worker threads before process exit.
