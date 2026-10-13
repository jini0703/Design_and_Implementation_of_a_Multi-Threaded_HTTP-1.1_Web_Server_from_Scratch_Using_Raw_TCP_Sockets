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
