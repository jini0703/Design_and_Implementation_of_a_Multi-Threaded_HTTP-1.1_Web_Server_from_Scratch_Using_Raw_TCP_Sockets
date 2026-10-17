# Experimental Methodology & Performance Evaluation Plan

## 1. Research Objectives & Evaluation Goals

The primary systems-engineering objective of this project is to quantitatively measure the performance, concurrency characteristics, and overload resilience of a from-scratch HTTP/1.1 server.

Specifically, the evaluation aims to answer three core research questions:
1. **Concurrency Scalability**: How does throughput (requests/sec) and latency scale as worker threads increase from $1$ to $16$ across varying client concurrency levels?
2. **Persistent Connection Efficiency**: What quantifiable reduction in latency and CPU utilization is achieved by reusing TCP connections via HTTP/1.1 Keep-Alive compared to establishing a fresh TCP handshake per request?
3. **Adaptive Overload Resilience**: Does the proposed **Queue-and-Latency-Aware Admission Control (QLAAC)** mechanism successfully prevent latency collapse and accelerate recovery during severe burst overloads compared to a static queue baseline?

---

## 2. Experimental Test Matrix

The benchmark test grid evaluates the system across five systematic experiments:

```text
+---------------+------------------------+------------------------------------+---------------------------------------+
| Experiment    | Category               | Independent Variables              | Target Metrics                        |
+---------------+------------------------+------------------------------------+---------------------------------------+
| Exp A         | Concurrency Scaling    | Workers: 1, 2, 4, 8, 16            | Throughput (req/sec), Latency p50/p95 |
|               |                        | Clients: 1, 10, 50, 100, 200       | CPU Utilization (%)                   |
+---------------+------------------------+------------------------------------+---------------------------------------+
| Exp B         | Adaptive Overload      | Server Policy: Baseline vs. QLAAC  | Tail Latency (p95/p99), Rejections,   |
|               | (QLAAC Evaluation)     | Traffic: Step-burst & Sustained    | Mean Time to Recovery (MTTR)          |
+---------------+------------------------+------------------------------------+---------------------------------------+
| Exp C         | Persistent Connections | Mode: Keep-Alive vs. Close         | Total TCP Handshakes, Latency CDF,    |
|               |                        | Concurrency: 10, 50, 100           | Throughput (req/sec)                  |
+---------------+------------------------+------------------------------------+---------------------------------------+
| Exp D         | Workload / Payload     | Resource Size: 1 KB, 100 KB, 5 MB  | Transfer Rate (MB/sec), CPU time,     |
|               | Variations             | Mode: User-space write vs sendfile | Context Switch Count                  |
+---------------+------------------------+------------------------------------+---------------------------------------+
| Exp E         | Defensive Reliability  | Malformed requests, Traversal attempts, | Error distribution, Socket cleanup, |
|               |                        | Slow-client trickle (Slowloris)    | Worker thread starvation rate         |
+---------------+------------------------+------------------------------------+---------------------------------------+
```

---

## 3. Comparative Evaluation: Baseline vs. QLAAC

### 3.1 Static-Policy Baseline
- Operates with a fixed worker pool and a standard bounded FIFO connection queue.
- If the queue reaches capacity, incoming connections block or are dropped once the kernel backlog overflows.
- Does not monitor response latency or preemptively shed load.

### 3.2 Proposed Adaptive Mechanism (QLAAC)
- Continuously samples queue depth and rolling $p95$ response latency every sampling window (e.g. 1000 ms).
- **NORMAL State**: All connections admitted; standard keep-alive allowed.
- **CAUTION State**: Triggered when queue occupancy exceeds 70% or $p95$ latency breaches 50 ms. Throttles keep-alive reuse to rapidly free worker threads.
- **OVERLOAD State**: Triggered when queue occupancy exceeds 90% or $p95$ latency breaches 150 ms. Immediately sheds newly accepted connections with rapid `503 Service Unavailable` responses before threads become saturated.
- **Hysteresis Recovery**: Requires 5 consecutive healthy sampling windows before stepping down to CAUTION or NORMAL.

---

## 4. Measurement Rigor & Statistical Methodology

1. **Repetition and Confidence**:
   - Each parameter combination is executed across a minimum of $5$ independent trials.
   - For all reported metrics, the mean, standard deviation, and 95% confidence intervals are computed.
2. **Environmental Control**:
   - Every benchmark report logs the complete system environment: CPU architecture, core topology, clock governor (performance mode), memory capacity, Linux kernel version, and network interface settings.
   - Experiments are conducted in dedicated execution windows with background daemon processes suspended.
3. **Addressing Coordinated Omission**:
   - Utilizing load generators (such as `wrk`) that record high-resolution latency histograms rather than simple arithmetic averages.
   - Capturing complete latency distributions: Median ($p50$), 90th percentile ($p90$), 95th percentile ($p95$), and 99th percentile ($p99$).

---

## 5. Data Pipeline & Artifact Storage

```text
[Load Generator / Orchestrator]
              │
              ▼
    results/raw/*.csv           (Unprocessed timestamped trial metrics)
              │
              ▼
   benchmark/process_data.py    (Aggregate means, standard deviations, and percentiles)
              │
              ▼
  results/processed/*.csv       (Structured summary tables for analysis)
              │
              ▼
    benchmark/plot_results.py   (Matplotlib / Seaborn visualization generation)
              │
              ▼
    results/plots/*.png         (Final publication-ready figures for technical report)
```

> **Data Integrity Policy**: All charts and performance tables in final academic deliverables must stem directly from verified CSV records in `results/`. No fabricated performance statistics may ever be introduced.

---

## 6. Corrected Benchmark Accounting & HTTP Metrics Specification (v2.0)

In benchmark harness v2.0 (`benchmark/run_experiment.py`), naive substring matching (`if b"200 OK" in resp or b"HTTP/1.1" in resp:`) has been completely replaced with RFC 9112 status-line parsing, rigorous response framing, and separated metric counters.

### 6.1 CSV Metrics Schema

| Metric Column | Type | Mathematical / Logical Definition | Description |
|:---|:---|:---|:---|
| `attempted_requests` | uint64 | $\sum \text{requests sent}$ | Total HTTP request payloads written to the client socket. |
| `completed_http_responses` | uint64 | $\text{success\_2xx} + \text{3xx} + \text{4xx} + \text{5xx}$ | Total fully framed, valid HTTP status lines received. |
| `success_2xx` | uint64 | Count of status codes $\in [200, 299]$ | Verified application successes (e.g. 200 OK). |
| `http_3xx` | uint64 | Count of status codes $\in [300, 399]$ | Redirection responses (e.g. 301, 304). |
| `http_4xx` | uint64 | Count of status codes $\in [400, 499]$ | Client errors (e.g. 400 Bad Request, 404 Not Found, 405). |
| `http_5xx` | uint64 | Count of status codes $\in [500, 599]$ | Server errors and QLAAC overload shedding (e.g. 503 Service Unavailable). |
| `transport_errors` | uint64 | Count of TCP RST, broken pipe, connection refused | Transport-layer faults before a valid HTTP response could be received. |
| `timeouts` | uint64 | Count of connect / read socket timeouts | Socket deadline exceeded without receiving expected bytes. |
| `invalid_responses` | uint64 | Count of framing errors / truncated bodies | Malformed headers, missing CRLF CRLF, or premature EOF mid-body. |
| `successful_rps` | float | $\frac{\text{success\_2xx}}{\text{duration\_sec}}$ | Strict throughput of verified successful application transactions. |
| `avg_latency_ms` | float / str | $\frac{1}{N} \sum_{i=1}^N \text{latency}_i$ | Arithmetic mean response latency for verified 2xx transactions (`N/A` if zero 2xx responses). |
| `p50_latency_ms` | float / str | 50th percentile of $\text{latencies}_{2xx}$ | Median response duration for 2xx transactions (`N/A` if zero 2xx responses). |
| `p95_latency_ms` | float / str | 95th percentile of $\text{latencies}_{2xx}$ | Tail latency indicating 95% of 2xx requests completed within this time (`N/A` if zero 2xx responses). |
| `p99_latency_ms` | float / str | 99th percentile of $\text{latencies}_{2xx}$ | Severe tail latency indicating 99% of 2xx requests completed within this time (`N/A` if zero 2xx responses). |

### 6.2 Stream Framing & Persistent Connection Integrity
TCP is a byte-stream protocol where packets may fragment or coalesce. The benchmark client `read_http_response` enforces:
1. **Header Draining**: Accumulates socket chunks until the delimiter `\r\n\r\n` is encountered.
2. **Status-Line Extraction**: Extracts the exact HTTP numeric status code (`int(status_parts[1])`).
3. **Framing & Content-Length Draining**: Drains exactly `Content-Length` bytes from the socket.
4. **Leftover Buffer Preservation**: Any trailing bytes received past the current response body are retained in `remaining_buf` and passed to subsequent requests on the persistent connection, preventing body bleeding across consecutive transactions.
5. **Connection: close Handling**: If `Connection: close` is present or status code dictates closure, the persistent loop terminates and cleanly closes the socket.

### 6.3 Server-Side Overload Accounting Invariants
In `src/server.c` and `src/metrics.c`:
- **Connection Rejection vs. Request Completion**: A rejected TCP connection does not equal an application request. If the server sheds load under QLAAC or bounded queue overflow:
  - If the HTTP 503 response is successfully transmitted over the raw TCP socket, `status_5xx_count`, `total_overload_503_sent`, and `bytes_sent` are incremented.
  - Crucially, `total_requests_completed` is **not** incremented, maintaining strict separation between fully processed application requests and transport-level overload shedding.
  - If socket transmission fails (e.g. client disconnected), only `total_connections_rejected` increments; the server does not report a successful 503 transmission.

---

## 7. Limitations of Historical Benchmark Data

Prior to repository correction (benchmark harness v2.0):
1. **False Success Accounting**: Legacy benchmark scripts evaluated responses via:
   ```python
   if b"200 OK" in resp or b"HTTP/1.1" in resp:
   ```
   Because HTTP 503 responses begin with `HTTP/1.1 503 Service Unavailable`, overload shedding responses were incorrectly recorded as successful responses. This artificially inflated successful RPS metrics under heavy load.
2. **Framing & Buffer Bleeding**: The legacy benchmark assumed a single `recv()` call contained exactly one response. On persistent connections, response bodies could bleed into subsequent requests, leading to unhandled exceptions.
3. **Historical Data Status**:
   - Files recorded under the legacy logic (such as `results/raw/benchmark_results.csv`, `fixed_policy_8080.csv`, `fixed_policy_high_load.csv`, `qlaac_enabled_8081.csv`, and `qlaac_enabled_high_load.csv`) are preserved for auditability but must be treated strictly as **preliminary data**.
   - These legacy files are **not directly comparable** to corrected results generated under the v2.0 schema.
   - All comparative evaluations, charts, and final project deliverables must strictly use datasets collected via the corrected harness (written by default to `results/raw/benchmark_results_corrected.csv` or explicitly specified new target files).
