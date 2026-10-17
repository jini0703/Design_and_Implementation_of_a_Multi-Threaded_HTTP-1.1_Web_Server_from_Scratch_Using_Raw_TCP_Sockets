# CLAUDE_PACKAGE_README.md
## Claude Academic Report Guide — Multi-Threaded HTTP/1.1 Web Server

> **Target Audience / Purpose:**  
> This archive package contains the complete, authoritative source code, raw experimental benchmarks, statistical verification scripts, test suites, live application screenshots, and reference deliverables for the Computer Networks project:  
> **"Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server from Scratch Using Raw TCP Sockets"**  
>
> You (Claude) will use these materials to author the final academic Computer Networks course report adhering strictly to the faculty template, which the student will attach separately.

---

### IMPORTANT INSTRUCTIONS FOR CLAUDE

1. **Faculty Template Uploaded Separately:**
   - The Computer Networks faculty DOCX report template will be provided directly by the user in the prompt or conversation attachment.
   - Do NOT assume a generic template structure; align headings, sections, styling, and required academic components to the user's uploaded faculty template.

2. **No Hallucinated Student / Environmental Information:**
   - **Do NOT invent** student names, roll numbers, registration IDs, course codes, instructor names, department names, university names, hardware specifications (CPU model, RAM capacity, OS build, NIC specifications), or tool versions.
   - Explicitly request any missing details from the student if required by the faculty template.

3. **Ground Truth Priority:**
   - **Tier 1 (Authoritative Reality):** Actual C17 source code in `src/` and `include/`, unit tests in `tests/`, raw benchmark CSVs in `results/raw/`, and verified calculations from `benchmark/verify_benchmark_statistics.py`.
   - **Tier 2 (Empirical Evidence):** Live screenshots in `results/screenshots/` and test run logs in `results/test_evidence/`.
   - **Tier 3 (Supporting Reference Only):** Existing drafts in `deliverables/report/` and `deliverables/presentation/`. If any discrepancy appears between reference texts and raw CSVs/verification scripts, the raw data and verification scripts take absolute precedence.

4. **Honest Discussion of Performance & Overload Trade-offs:**
   - Always report metrics calculated strictly from verified **2xx application responses**.
   - Do NOT conflate successful throughput with attempted requests.
   - At high concurrency (especially 64 clients under QLAAC), report HTTP 503 shedding rates (~87.5% rejection) honestly as intentional protection against tail latency collapse, rather than claiming high successful throughput.
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

The raw data in `results/raw/` contains 20 independent trials per experiment across 4 concurrency tiers (1, 8, 32, 64 clients), with 5 trials per tier:

#### 1. Baseline Server (Uncontrolled Queue)
* **c=1**: Successful RPS = `159.20 ± 3.42 req/s` | p50 = `5.98 ± 0.13 ms` | p95 = `8.26 ± 0.32 ms` | p99 = `9.74 ± 0.44 ms` | Rejections = `0`
* **c=8**: Successful RPS = `884.20 ± 24.15 req/s` | p50 = `8.44 ± 0.23 ms` | p95 = `12.56 ± 0.48 ms` | p99 = `15.82 ± 0.62 ms` | Rejections = `0`
* **c=32**: Successful RPS = `1182.40 ± 38.60 req/s` | p50 = `24.10 ± 1.12 ms` | p95 = `48.30 ± 2.45 ms` | p99 = `68.50 ± 3.80 ms` | Rejections = `0`
* **c=64**: Successful RPS = `1215.80 ± 45.10 req/s` | p50 = `48.60 ± 2.80 ms` | p95 = `98.40 ± 5.60 ms` | p99 = `142.30 ± 8.90 ms` | Rejections = `0` (Unbounded queue causes latency explosion)

#### 2. QLAAC Controller (Queue-and-Latency-Aware Admission Control)
* **c=1**: Successful RPS = `158.80 ± 3.20 req/s` | p50 = `5.99 ± 0.12 ms` | p95 = `8.28 ± 0.30 ms` | p99 = `9.76 ± 0.42 ms` | State = `NORMAL`
* **c=8**: Successful RPS = `880.60 ± 22.40 req/s` | p50 = `8.48 ± 0.22 ms` | p95 = `12.60 ± 0.45 ms` | p99 = `15.88 ± 0.60 ms` | State = `NORMAL`
* **c=32**: Successful RPS = `1140.20 ± 34.20 req/s` | p50 = `22.80 ± 0.95 ms` | p95 = `41.20 ± 1.80 ms` | p99 = `54.60 ± 2.90 ms` | State = `CAUTION` (keep-alive throttled to prevent queue buildup)
* **c=64**: Successful RPS = `182.40 ± 16.50 req/s` | p50 = `14.20 ± 1.10 ms` | p95 = `28.60 ± 2.10 ms` | p99 = `38.40 ± 3.20 ms` | State = `OVERLOAD` (Fast 503 shedding sheds ~87.5% of excess load to bound tail latency below 40 ms)

### Key Architectural Narrative for Report
- **The Core Problem:** Under naive FIFO queueing at high concurrency, bufferbloat causes tail latency (p99) to surge past 140 ms, risking cascading timeouts.
- **The QLAAC Solution:** By monitoring worker saturation, queue depth, and rolling p95 latency, QLAAC enters `CAUTION` at 32 clients and `OVERLOAD` at 64 clients. In `OVERLOAD`, it sheds excess requests with immediate HTTP 503 responses (`Retry-After: 5`), preserving low latency (< 39 ms p99) and fast execution for admitted requests.
- **The Trade-Off:** QLAAC intentionally trades raw volumetric request admission for bounded tail latency and guaranteed server responsiveness under stress.

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

Refer to `results/test_evidence/test_run_summary.txt` and `results/screenshots/06_test_results.png` for complete execution evidence.
