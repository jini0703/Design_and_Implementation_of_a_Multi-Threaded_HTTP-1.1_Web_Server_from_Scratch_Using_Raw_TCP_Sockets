# Application Verification Screenshots & Runtime Visual Evidence

This directory contains authentic visual evidence captured directly from the running C17 Multi-Threaded HTTP/1.1 Web Server and its integrated systems monitoring dashboard.

All five screenshots were captured from a live server instance running under POSIX/Linux (WSL2 environment) communicating with the browser frontend via raw TCP sockets at `http://127.0.0.1:8080/`.

---

## Screenshot Inventory & Descriptions

### 1. `01_dashboard_overview.png`
* **Subject**: Full Dashboard Overview & Systems Monitoring Interface
* **Execution & Capture Conditions**: Captured at initial server steady state with `QLAAC: ENABLED` and `Online (HTTP/1.1 Live)`.
* **Demonstrated Behavior**:
  - **Header & Health Indicators**: Real-time server connectivity status badge, active QLAAC policy indicator (`QLAAC: ENABLED`), and polling timestamp.
  - **Core Telemetry Grid**:
    - **Throughput (RPS)**: Measured rolling window rate alongside lifetime request totals.
    - **Response Latency**: 2xx percentile metrics (p50, p95, p99, average) displaying the verified "Idle — no recent samples" state with null-break rendering when no recent requests occurred.
    - **Worker Thread Pool**: Pre-allocated POSIX pthread pool utilization (4 worker threads).
    - **Bounded Queue Depth**: Thread-safe circular FIFO buffer occupancy (capacity 64 connections).
    - **Connection Counters**: Cumulative accepted TCP connections, connection rejections, and HTTP 503 overload shedding events.
    - **Server Uptime & Network I/O**: Telemetry tracking elapsed server uptime, transmitted bytes, and received bytes.
  - **Dynamic Canvas Charts**: Dual real-time Canvas charts tracking rolling throughput history (RPS) and tail latency history (p50, p95, p99).

---

### 2. `02_health_check.png`
* **Subject**: Live Diagnostic Endpoint Verification — `GET /health`
* **Execution & Capture Conditions**: Dispatched directly from the dashboard's interactive API console against the running server.
* **Demonstrated Behavior**:
  - Displays the live HTTP response status line `HTTP/1.1 200 OK` (round-trip latency: ~4.50 ms).
  - Validates `Content-Type: application/json; charset=utf-8` header generation.
  - Confirms JSON serialization of the server health payload:
    ```json
    {
      "status": "healthy",
      "server": "HTTP-Server-C17/1.0.0",
      "protocol": "HTTP/1.1"
    }
    ```
  - Displays the HTTP Response Status Distribution bar separating 2xx application successes from 4xx client errors and 5xx overload responses.

---

### 3. `03_post_echo.png`
* **Subject**: HTTP POST Body Parsing & Echo Endpoint — `POST /api/echo`
* **Execution & Capture Conditions**: Transmitted from the interactive diagnostic console with an explicit payload (`"Hello from systems dashboard live verification!"`).
* **Demonstrated Behavior**:
  - Displays HTTP status `HTTP/1.1 200 OK` (round-trip latency: ~5.30 ms).
  - Validates RFC 9112 Content-Length body framing and socket buffer consumption.
  - Displays exact echo verification in `Content-Type: text/plain; charset=utf-8`:
    ```
    Hello from systems dashboard live verification!
    ```

---

### 4. `04_live_metrics.png`
* **Subject**: Real-Time Telemetry & Chart Response Under Local Workload
* **Execution & Capture Conditions**: Captured immediately following a controlled burst of 120 client requests dispatched to `/health`.
* **Demonstrated Behavior**:
  - Throughput history chart dynamically plots the surge, peaking at ~46 RPS.
  - Response Latency card updates with live 2xx percentiles:
    - **p50**: `0.12 ms`
    - **p95**: `0.44 ms`
    - **p99**: `0.82 ms`
    - **Average**: `0.15 ms`
  - Latency profile chart immediately resumes active plotting of p50, p95, and p99 percentile trajectories, recovering seamlessly from the previous idle state.
  - Connection counters dynamically register new accepted requests (3,565 total requests, 414 accepted connections).

---

### 5. `05_qlaac_load_test.png`
* **Subject**: Queue-and-Latency-Aware Admission Control (QLAAC) Telemetry & Policy Invariants
* **Execution & Capture Conditions**: Captured showing the dedicated QLAAC controller telemetry grid and state machine specification.
* **Demonstrated Behavior**:
  - **Controller State Card**: Visual badge indicating active state (`NORMAL`), state description, overload transition count, and 503 shedding counter.
  - **Dynamic Threshold Signals**: Real-time signal bars tracking:
    - Worker Saturation: Active worker ratio (`25.0%` with green status fill).
    - Queue Occupancy: Connection FIFO buffer fill percentage (`0.0%`).
    - Recent p95 Tail Latency: Sampled rolling window latency.
  - **State Machine Invariants**: Documents formal threshold transitions between `NORMAL` (safe), `CAUTION` (keep-alive throttling with `Connection: close`), and `OVERLOAD` (immediate 503 shedding with `Retry-After: 5` and hysteresis dampening).

---

## Test Verification Note

Automated test execution evidence is verified and documented in textual format in [`results/test_evidence/test_run_summary.txt`](../test_evidence/test_run_summary.txt) and verifiable via `make test` (44 C unit tests) and `python3 -m unittest discover -s tests -p "test_*.py"` (48 Python integration tests). All 92 automated tests pass with 100% success against an active server.
