# HTTP/1.1 Protocol Support & Standards Compliance

## 1. Reference Specification

This project targets a well-defined, robust subset of the **Hypertext Transfer Protocol (HTTP/1.1)** as specified in:

- **RFC 9112**: *HTTP/1.1 (June 2022)* — [https://www.rfc-editor.org/rfc/rfc9112.html](https://www.rfc-editor.org/rfc/rfc9112.html)
- **RFC 9110**: *HTTP Semantics (June 2022)* — [https://www.rfc-editor.org/rfc/rfc9110.html](https://www.rfc-editor.org/rfc/rfc9110.html)

The implementation aims for rigorous protocol correctness within its documented functional boundary. Claims of full standards compliance are explicitly avoided until all protocol behaviors are implemented and verified via automated test suites.

---

## 2. Supported HTTP Methods

| Method | Support Tier | Behavior & Implementation Details |
| :--- | :--- | :--- |
| **`GET`** | Core (Planned) | Retrieves representations of specified resources. Supports static file delivery and dynamic API endpoints (e.g. `/metrics`). |
| **`HEAD`** | Core (Planned) | Identical to `GET`, but omits the response message body. Accurately calculates and sends `Content-Length` and headers without writing body payload. |
| **`POST`** | Core Bounded (Planned) | Accepts request payloads bounded by `max_body_size` (default: 10 MB). Requires unambiguous `Content-Length` header. |
| **`PUT` / `DELETE` / `PATCH`** | Rejected | Returns `405 Method Not Allowed` with an `Allow: GET, HEAD, POST` response header. |
| **`OPTIONS` / `TRACE` / `CONNECT`** | Rejected | Returns `501 Not Implemented` or `405 Method Not Allowed`. |

---

## 3. Protocol Grammar & Framing Rules

### 3.1 Request-Line Parsing
The request line must strictly adhere to the ABNF grammar specified in RFC 9112 Section 3:
```abnf
request-line = method SP request-target SP HTTP-version CRLF
```
- **Method**: Must consist of token characters (`ALPHA / DIGIT / special`).
- **Request-Target**: Must begin with `/` (origin-form). Targets exceeding `HTTP_MAX_URI_LENGTH` (2,048 bytes) immediately trigger `414 URI Too Long`.
- **HTTP-Version**: Server strictly verifies `HTTP/1.1` (with backward tolerance for `HTTP/1.0`). Unsupported version tokens trigger `505 HTTP Version Not Supported` or `400 Bad Request`.
- **Line Terminator**: Strict requirement for CRLF (`\r\n`). Bare LF (`\n`) is rejected to mitigate request smuggling ambiguities.

### 3.2 Header Field Semantics
- **Field Grammar**: `field-name ":" OWS field-value OWS CRLF`
- **Case-Insensitivity**: Header field names are parsed case-insensitively using `strcasecmp()` (e.g., `content-type`, `Content-Type`, and `CONTENT-TYPE` match identically).
- **Whitespace Handling**: Leading and trailing optional whitespace (OWS) around field values is stripped.
- **Header Limits**: Maximum 64 headers per request. Cumulative header size must not exceed `max_header_size` (default: 8,192 bytes); violations trigger `431 Request Header Fields Too Large` or `400 Bad Request`.

### 3.3 Message Body Framing & Bounds
- **Content-Length**: Must be a valid non-negative integer. If `Content-Length` exceeds `max_body_size`, the server rejects the request with `413 Payload Too Large`.
- **Framing Ambiguity**: If multiple conflicting `Content-Length` headers are supplied, the request is immediately rejected with `400 Bad Request` (RFC 9112 Section 6.1).
- **Transfer-Encoding**: Chunked transfer coding is deferred to future extensions. If a client transmits `Transfer-Encoding: chunked`, the initial implementation returns `501 Not Implemented` rather than attempting unsafe framing guesses.

---

## 4. Persistent Connections (HTTP Keep-Alive)

HTTP/1.1 defines persistent connections as the default behavior:

1. **Default Persistence**: Connections remain open after completing a request unless either client or server explicitly includes `Connection: close`.
2. **Sequential Requests**: Requests over persistent connections are processed sequentially per socket. Pipeline interleaving without framing verification is prevented.
3. **Keep-Alive Limits**:
   - `keepalive_max_requests`: Server limits the maximum consecutive requests on a single connection (default: 100) before issuing `Connection: close` and terminating the socket.
   - `keepalive_timeout_sec`: Inactivity timer (default: 5 seconds). If a client fails to transmit the next request within this window, the worker closes the socket cleanly.

---

## 5. Supported Response Status Codes

| Code | Reason Phrase | Typical Trigger Condition |
| :--- | :--- | :--- |
| **`200`** | `OK` | Successful retrieval of resource or execution of GET/POST. |
| **`204`** | `No Content` | Request succeeded; no payload returned. |
| **`301`** | `Moved Permanently` | Path canonicalization or trailing slash redirection. |
| **`304`** | `Not Modified` | Planned conditional validation cache match. |
| **`400`** | `Bad Request` | Malformed request line, invalid header syntax, or ambiguous framing. |
| **`403`** | `Forbidden` | Access denied, unreadable file permissions, or directory listing prohibited. |
| **`404`** | `Not Found` | Requested URI does not exist within public root or router table. |
| **`405`** | `Method Not Allowed` | Unrecognized or unsupported HTTP verb on valid resource. |
| **`408`** | `Request Timeout` | Client failed to complete header or body transmission before socket timeout. |
| **`413`** | `Payload Too Large` | Request body exceeds configured `max_body_size`. |
| **`414`** | `URI Too Long` | Request URI exceeds 2,048 bytes. |
| **`431`** | `Request Header Fields Too Large` | Headers exceed configured 8,192 byte limit. |
| **`500`** | `Internal Server Error` | Unexpected system failure or I/O failure during processing. |
| **`501`** | `Not Implemented` | Unsupported transfer encodings or unimplemented HTTP methods. |
| **`503`** | `Service Unavailable` | Server overloaded; connection rejected by QLAAC or connection queue. |

---

## 6. Planned Scope vs. Future Extensions

| Feature | Initial Planned Core | Future Extension Phase |
| :--- | :---: | :---: |
| POSIX TCP Socket Listener | Yes | Completed |
| Bounded Pthread Worker Pool | Yes | Completed |
| Streaming RFC 9112 Request Parser | Yes | Completed |
| Static File Serving (`sendfile`) | Yes | Completed |
| Persistent Connections (`Keep-Alive`) | Yes | Completed |
| QLAAC Overload Control | Yes | Completed |
| Dynamic `/metrics` Endpoint | Yes | Completed |
| Chunked Transfer-Encoding (Decode) | No | Planned |
| Range Requests (`206 Partial Content`) | No | Planned |
| TLS / HTTPS Encryption | No | Planned |
| HTTP/2 Multiplexing | No | Out of Scope |
