# Performance Benchmarking & Experimental Evaluation

This directory contains the benchmarking harness, experiment configurations, load generation wrappers, and data processing scripts for evaluating the custom multi-threaded HTTP/1.1 web server.

---

## 1. Directory Structure

```text
benchmark/
├── README.md                 # Benchmarking methodology, tooling, and workflow guide
├── run_experiment.py         # Python test orchestration and load runner wrapper
└── configurations/           # Parameter sets for repeatable benchmark runs
    └── .gitkeep
```

Output directories (located at repository root):
```text
results/
├── raw/                      # Unprocessed CSV logs, wrk outputs, and system metrics
├── processed/                # Aggregated summary statistics and statistical summaries
└── plots/                    # Rendered figures (latency distributions, throughput curves)
```

---

## 2. Experimental Philosophy & Integrity Rules

1. **Empirical Ground Truth**: No performance figures or graphs may be fabricated or assumed. All reported metrics must originate from verified execution runs on documented hardware.
2. **Repeatability**: Every experimental configuration is executed across multiple trials (minimum 3–5 trials per parameter combination) to calculate standard deviations and eliminate transient system noise.
3. **Controlled Isolation**: Benchmarks must document CPU core counts, frequency governors, operating system kernel versions, and whether load generators run on loopback or separate hardware.
4. **Coordinated Omission Prevention**: Latency percentiles ($p50$, $p95$, $p99$) must be captured under saturated burst conditions to ensure queued requests are accurately represented.

---

## 3. Benchmarking Tooling

The evaluation framework relies on standard network performance tools:

- **`wrk`**: High-performance HTTP benchmarking tool capable of generating sustained concurrent connections using multithreaded epoll.
  - Install on Ubuntu: `sudo apt-get install wrk`
  - Upstream: [https://github.com/wg/wrk](https://github.com/wg/wrk)
- **`run_experiment.py`**: Custom Python orchestrator that boots the server under specified configurations, launches workload generators, parses output matrices, and appends structured rows to `results/raw/results.csv`.
- **`matplotlib` / `seaborn`** (Planned): Python visualization libraries used to generate boxplots, CDFs (Cumulative Distribution Functions), and throughput scaling curves.

---

## 4. Planned Experiment Matrix

| Experiment ID | Focus Area | Independent Variables | Measured Metrics |
| :--- | :--- | :--- | :--- |
| **EXP-A** | Concurrency Scaling | Worker Threads ($1, 2, 4, 8, 16$), Clients ($1, 10, 50, 100, 200$) | Throughput (req/sec), Median & $p95$ Latency |
| **EXP-B** | Overload & Admission Control | Fixed Queue Baseline vs. QLAAC Controller | Rejection Rate, Overload Recovery Time, Tail Latency |
| **EXP-C** | Connection Reuse | Keep-Alive Enabled vs. Connection: Close | TCP Handshake Overhead, Server CPU %, Latency |
| **EXP-D** | Payload Size Variations | 1 KB, 100 KB, 5 MB Static Files | Network Saturation, Sendfile Zero-Copy Efficiency |
| **EXP-E** | Defensive Resilience | Malformed Headers, Slowloris Trickle, Oversized Payloads | Socket Cleanup, Error Rate, System Stability |

---

## 5. Usage & Execution Instructions

> **Note on Implementation Status**: The commands below illustrate the intended execution workflow. Full end-to-end execution requires the active server binary (`bin/server`) compiled and listening.

### Running a Dry-Run Test Matrix

```bash
# Verify experiment orchestrator arguments and environment
python3 benchmark/run_experiment.py --help
```

### Planned Benchmark Execution (Upon Server Completion)

```bash
# Execute Experiment A (Concurrency Scaling) across 5 trials
python3 benchmark/run_experiment.py \
    --server-bin bin/server \
    --experiment concurrency \
    --threads 4 \
    --connections 10,50,100,200 \
    --duration 10s \
    --output results/raw/exp_a_concurrency.csv
```

### Processing Results into Visualizations

```bash
# Future visualization script target:
python3 benchmark/plot_results.py \
    --input results/raw/exp_a_concurrency.csv \
    --output-dir results/plots/
```
