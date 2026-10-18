# CLAUDE_PACKAGE_README.md
## Claude Academic Report Guide — Multi-Threaded HTTP/1.1 Web Server

> **Target Audience / Purpose:**  
> This archive package contains the complete, authoritative source code, raw experimental benchmarks, statistical verification scripts, test suites, live application screenshots, and reference deliverables for the Computer Networks project:  
> **"Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server from Scratch Using Raw TCP Sockets"**  
>
> You (Claude) will use these materials to author the final academic Computer Networks course report adhering strictly to the faculty template, which the student will attach separately.

### Project Team

| Registration Number | Student Name |
| :--- | :--- |
| 24BCE0702 | Yash Pradhan |
| 24BCE0714 | Anjini Pandey |

*Course:* Computer Networks Lab (BCSE308P / CS-504) — Fall Semester 2026–27

---

### IMPORTANT INSTRUCTIONS FOR CLAUDE

1. **Faculty Template Uploaded Separately:**
   - The Computer Networks faculty DOCX report template will be provided directly by the user in the prompt or conversation attachment.
   - Do NOT assume a generic template structure; align headings, sections, styling, and required academic components to the user's uploaded faculty template.

2. **Project Team & Institutional Information:**
   - The authorized project team consists of the two students listed above (**24BCE0702 — Yash Pradhan** and **24BCE0714 — Anjini Pandey**), listed in ascending registration-number order.
   - **Do NOT invent** other student names, roll numbers, instructor names, or unconfirmed department/university details.
   - Explicitly request any missing environmental details (such as specific evaluation rubrics or instructor names) from the student if required by the faculty template.

3. **Ground Truth Priority:**
   - **Tier 1 (Authoritative Reality):** Actual C17 source code in `src/` and `include/`, unit tests in `tests/`, raw benchmark CSVs in `results/raw/`, and verified calculations from `benchmark/verify_benchmark_statistics.py`.
   - **Tier 2 (Empirical Evidence):** Live screenshots in `results/screenshots/` and test run logs in `results/test_evidence/`.
   - **Tier 3 (Supporting Reference Only):** Existing drafts in `deliverables/report/` and `deliverables/presentation/`. If any discrepancy appears between reference texts and raw CSVs/verification scripts, the raw data and verification scripts take absolute precedence.

4. **Honest Discussion of Performance & Overload Trade-offs:**
   - Always report metrics calculated strictly from verified **2xx application responses**.
   - Do NOT conflate successful throughput with attempted requests.
   - At high concurrency (especially 64 clients under QLAAC), report HTTP 503 shedding rates (~72.14% rejection; 16,475.40 ± 969.70 5xx responses) honestly as intentional protection against tail latency collapse, rather than claiming high successful throughput.
   - Latency percentiles (p50, p95, p99) represent successful 2xx responses; when zero 2xx responses succeed, latency is mathematically undefined (`N/A`), not `0.00 ms`.

---

## Directory Structure & Role Guide

```
claude_report_package/
├── CLAUDE_PACKAGE_README.md       # [This Guide] Primary orientation for Claude
├── README.md                      # Project overview, architecture, quickstart, and design specs
├── Makefile                       # POSIX C17 compilation and test runner configuration
│
├── src/                           # Core C17 systems implementation modules
│   ├── main.c                     # Server entry point, CLI flag parsing, signal handling
│   ├── server.c                   # Connection accept loop, thread dispatch, keep-alive state
│   ├── tcp_socket.c               # Raw POSIX TCP sockets, non-blocking polling, socket options
│   ├── thread_pool.c              # Fixed-size pthread worker pool lifecycle
│   ├── connection_queue.c         # Thread-safe bounded circular FIFO (mutex + condvars)
│   ├── http_parser.c              # Zero-copy streaming RFC 9112 HTTP/1.1 request parser
│   ├── http_response.c            # RFC 9112 response builders, status codes, standard headers
│   ├── router.c                   # REST endpoint dispatcher (/health, /metrics, /api/echo)
│   ├── static_files.c             # sendfile(2) zero-copy static asset delivery & security checks
│   ├── adaptive_controller.c      # Queue-and-Latency-Aware Admission Control (QLAAC)
│   ├── metrics.c                  # Rolling window latency tracking, atomic counters, JSON exporter
│   └── util.c                     # Monotonic time helpers, RFC 1123 date formatting, strings
│
├── include/                       # Modular C header definitions and public APIs
│   ├── server.h, tcp_socket.h, thread_pool.h, connection_queue.h,
│   ├── http_parser.h, http_response.h, router.h, static_files.h,
│   ├── adaptive_controller.h, metrics.h, util.h, config.h
│
├── public/                        # Static web assets & live frontend dashboard
│   ├── index.html                 # Complete single-page systems monitoring dashboard (HTML/CSS/JS)
│   └── (other static assets)
│
├── config/                        # Server configuration files
│   └── server.conf.example        # Reference configuration with safe defaults
│
├── docs/                          # Comprehensive technical design documentation
│   ├── architecture.md            # Modular concurrency and data-plane architecture
│   ├── experiment-methodology.md  # Formal experimental protocol and measurement rules
│   ├── qlaac-state-machine.md     # Mathematical model, invariants, and hysteresis dampening
│   ├── rfc-compliance.md          # RFC 9112 / RFC 9110 compliance matrix
│   └── security-model.md          # Threat model, input validation, directory traversal prevention
│
├── tests/                         # Automated verification test suites
│   ├── test_controller.c          # C unit tests for QLAAC state transitions & hysteresis
│   ├── test_parser.c              # C unit tests for RFC 9112 compliance & error paths
│   ├── test_queue.c               # C unit tests for thread-safe circular FIFO bounds
│   ├── test_responses.c          # C unit tests for HTTP response builders
│   ├── test_server_metrics.c      # C unit tests for bounded rolling latency windows
│   ├── test_benchmark_accounting.py # Protocol tests verifying 2xx/5xx classification & accounting
│   ├── test_keepalive.py          # Integration tests for persistent TCP connections
│   ├── test_malformed_requests.py # Fuzzing & malformed request rejection tests
│   └── test_security.py           # Path traversal and HTTP method restriction tests
│
├── results/
│   ├── raw/                       # Raw experimental benchmark datasets (CSV format)
│   │   ├── experiment_baseline_tuned_comparison.csv  # 20 trials: Baseline (no QLAAC)
│   │   └── experiment_qlaac_tuned_comparison.csv     # 20 trials: QLAAC Active
│   │
│   ├── plots/                     # High-resolution benchmark comparison charts
│   │   ├── fig1_throughput_comparison.png     # Successful 2xx RPS comparison
│   │   ├── fig2_latency_p99_comparison.png      # Tail latency (p99) comparison
│   │   ├── fig3_qlaac_admission_tradeoff.png    # Admission rate vs. latency trade-off
│   │   ├── fig4_system_architecture.png         # Modular system architecture diagram
│   │   └── fig5_qlaac_state_machine.png         # QLAAC state transition diagram
│   │
│   ├── screenshots/               # Real application visual evidence from live server
│   │   ├── README.md              # Detailed explanations of test conditions & captures
│   │   ├── 01_dashboard_overview.png   # Full web dashboard overview & telemetry
│   │   ├── 02_health_check.png         # GET /health 200 OK & JSON serialization
│   │   ├── 03_post_echo.png            # POST /api/echo body framing & response
│   │   ├── 04_live_metrics.png         # Dynamic RPS spike & chart updates under load
│   │   ├── 05_qlaac_load_test.png      # QLAAC state, dynamic signals & invariants
│   │   └── 06_test_results.png         # Genuine terminal test output (49/49 passed)
│   │
│   └── test_evidence/             # Test execution summaries
│       └── test_run_summary.txt   # Verified pass/fail execution log of all test suites
│
├── benchmark/                     # Experimental benchmarking harness
│   ├── run_experiment.py          # Benchmark driver with strict HTTP status-line parsing
│   ├── verify_benchmark_statistics.py # Independent statistical verification script
│   └── generate_plots.py          # Visualization script producing publication-ready figures
│
└── deliverables/                  # Pre-compiled reference deliverables
    ├── report/
    │   ├── HTTP11_Web_Server_Project_Report.md   # Reference Markdown technical report
    │   ├── HTTP11_Web_Server_Project_Report.pdf  # Compiled reference PDF report
    │   ├── generate_report.py                    # Script generating Markdown report
    │   └── README.md                             # Report compilation notes
    └── presentation/
        ├── HTTP11_Web_Server_Project_Presentation.pptx # Reference slide deck (16:9)
        ├── generate_presentation.py                    # Slide generation script
        └── README.md                                   # Presentation notes
```

---

## Benchmark Metrics & Statistical Verification

Before quoting any numeric metric in the final report, execute or inspect:
```bash
python benchmark/verify_benchmark_statistics.py
```

### Verified Raw Benchmark Data Summary

The raw data in `results/raw/` contains 20 repeated trials per experiment across 4 concurrency tiers (2, 8, 32, 64 clients), with 5 repeated 10-second trials per tier (calculated via `benchmark/verify_benchmark_statistics.py` directly from `results/raw/experiment_*_tuned_comparison.csv` with sample standard deviation):

#### 1. Baseline Server (Bounded FIFO Queue, Capacity 64, No QLAAC)
* **c=2**: Successful 2xx RPS = `13,929.24 ± 334.23 req/s` | p50 = `0.13 ± 0.01 ms` | p95 = `0.23 ± 0.01 ms` | p99 = `0.31 ± 0.02 ms` | 5xx = `0.00 ± 0.00` | Shedding = `0.00%`
* **c=8**: Successful 2xx RPS = `6,629.38 ± 148.21 req/s` | p50 = `0.54 ± 0.01 ms` | p95 = `1.38 ± 0.04 ms` | p99 = `29.09 ± 0.50 ms` | 5xx = `0.00 ± 0.00` | Shedding = `0.00%`
* **c=32**: Successful 2xx RPS = `6,860.38 ± 107.11 req/s` | p50 = `0.52 ± 0.01 ms` | p95 = `1.34 ± 0.03 ms` | p99 = `204.00 ± 2.94 ms` | 5xx = `0.00 ± 0.00` | Shedding = `0.00%`
* **c=64**: Successful 2xx RPS = `6,828.52 ± 157.91 req/s` | p50 = `0.52 ± 0.01 ms` | p95 = `1.38 ± 0.05 ms` | p99 = `440.33 ± 9.21 ms` | 5xx = `0.00 ± 0.00` | Shedding = `0.00%` (Connections wait in bounded queue behind pinned workers, inflating tail latency)

#### 2. QLAAC Controller (Queue-and-Latency-Aware Admission Control)
* **c=2**: Successful 2xx RPS = `14,665.40 ± 610.30 req/s` | p50 = `0.12 ± 0.00 ms` | p95 = `0.22 ± 0.03 ms` | p99 = `0.33 ± 0.05 ms` | 5xx = `0.00 ± 0.00` | Shedding = `0.00%` | State = `NORMAL`
* **c=8**: Successful 2xx RPS = `2,129.26 ± 50.35 req/s` | p50 = `1.55 ± 0.02 ms` | p95 = `2.83 ± 0.03 ms` | p99 = `3.55 ± 0.06 ms` | 5xx = `0.00 ± 0.00` | Shedding = `0.00%` | State = `CAUTION` (keep-alive shortened)
* **c=32**: Successful 2xx RPS = `1,970.40 ± 55.83 req/s` | p50 = `10.46 ± 0.26 ms` | p95 = `14.31 ± 0.66 ms` | p99 = `16.55 ± 1.00 ms` | 5xx = `0.00 ± 0.00` | Shedding = `0.00%` | State = `CAUTION`
* **c=64**: Successful 2xx RPS = `638.12 ± 126.44 req/s` | p50 = `19.61 ± 6.35 ms` | p95 = `28.76 ± 0.59 ms` | p99 = `161.18 ± 159.88 ms` | 5xx = `16,475.40 ± 969.70` | Shedding = `72.14% ± 5.15%` | State = `OVERLOAD` (Early 503 shedding sheds 72.14% of excess load; caps p99 latency compared to baseline 440 ms, but exhibits high sample standard deviation of 159.88 ms due to cycling between OVERLOAD and CAUTION)

### Key Architectural Narrative for Report
- **The Core Problem:** Under naive FIFO queueing at high concurrency, connections queue up behind pinned workers, causing tail latency (p99) to surge to 440.33 ms at 64 clients.
- **The QLAAC Solution:** By monitoring worker saturation, queue depth, and rolling p95 latency, QLAAC enters `CAUTION` at 8 and 32 clients and `OVERLOAD` at 64 clients. In `OVERLOAD`, it sheds excess requests with immediate HTTP 503 responses (`Retry-After: 5`), capping tail latency compared to the baseline.
- **The Trade-Off:** QLAAC intentionally trades raw volumetric request throughput (-90.7% at 64 clients) for bounded tail latency, and its overload tail exhibits significant variance across repeated trials.

---

## Automated Test Verification Summary

The project includes 49 automated test cases covering systems-level C17 components and network protocol behaviors. All 49 tests pass with zero failures:
1. **C17 Unit Tests (26 tests via `make test`):**
   - QLAAC controller state transitions & recovery hysteresis (7 tests)
   - RFC 9112 streaming HTTP parser & security checks (9 tests)
   - Thread-safe circular FIFO queue (3 tests)
   - HTTP response generation (2 tests)
   - Server metrics & bounded latency window (5 tests)
2. **Python Protocol & Integration Tests (23 tests via `unittest`):**
   - Benchmark accounting & HTTP status classification (11 tests)
   - HTTP/1.1 persistent connections & keep-alive (3 tests)
   - Malformed request handling & syntax validation (5 tests)
   - Security constraints & path traversal prevention (4 tests)

Refer to `results/test_evidence/test_run_summary.txt` for complete authentic execution evidence (image `06_test_results.png` was styled with terminal typography and is superseded by the verified raw test summary).
