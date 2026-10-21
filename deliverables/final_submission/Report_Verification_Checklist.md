# Report Verification Checklist — BCSE308P CN Lab Project

**Files:** `CN_Project_Final_Report.docx` (editable), `CN_Project_Final_Report.pdf` (46-page PDF rendered from the corrected DOCX using LibreOffice 25.2.3.2), this checklist. The current PDF and static table of contents are aligned to this LibreOffice rendering; pagination should be checked again if Microsoft Word is used to export a replacement PDF.

---

## 1. Template Section → Report Location & Audit Status

| Template Requirement | Audit Status | Location in Report | Verification Details |
|---|---|---|---|
| **Cover Page** (Title 16 pt bold caps, particulars sorted by Reg. No., B.Tech., CSE, School, VIT logo, October 2026) | **COMPLETE & VERIFIED** | Page 1 | Sorted ascending: (1) 24BCE0702 — YASH PRADHAN, (2) 24BCE0714 — ANJINI PANDEY. Placeholders removed. VIT logo preserved. |
| **Abstract** (One page, $\le 300$ words) | **COUNT NEEDS WORD CONFIRMATION** | Page 2 | The current abstract has 297 whitespace-separated words and remains below the 300-word limit by that count. Confirm with Microsoft Word's word counter before submission. Test statement: 72 automated tests (37 C, 35 Python) pass with a server running. |
| **Certificate** (Official VIT template wording) | **COMPLETE & VERIFIED** | Page 3 | Both students and registration numbers listed; placeholders removed; signature line present. |
| **Table of Contents** (Static numbers matching the current PDF) | **MATCHES CURRENT RENDER; WORD CHECK PENDING** | Pages 4–5 | Static page numbers have been corrected and checked against the current 46-page LibreOffice PDF generated from this DOCX. Recheck page numbers if the DOCX is reflowed/exported with Microsoft Word. |
| **1. Introduction** (1.1–1.5) | **COMPLETE & VERIFIED** | Pages 6–8 | Section 1.5 editorial placeholder removed; formally mapped to core Computer Networks concepts. |
| **2. Literature Survey** (2.1–2.3, template table) | **COMPLETE & VERIFIED** | Pages 9–11 | 6 survey rows; research gap and novelty justification formally framed. |
| **3. Proposed System** (3.1–3.5, architecture, flowchart, pseudocode) | **COMPLETE & VERIFIED** | Pages 12–16 | Bounded queue (capacity 64) accurately documented; QLAAC 3-state machine and trade-offs explained. |
| **4. Implementation** (4.1–4.4) | **COMPLETE & VERIFIED** | Pages 18–26 | Table 4.3 updated to verified `wc -l` source counts; Table 4.4 expanded with confirmed security, framing, and accounting fixes; 72 automated tests verified. |
| **5. Evaluation** (5.1–5.5, results table, graphs, critical analysis) | **COMPLETE & VERIFIED** | Pages 27–34 | Primary empirical CSV dataset preserved (5 × 10 s, C = 2, 8, 32, 64); benchmark chronology clearly distinguished from subsequent regression tests; historical counter discrepancy qualified. |
| **6. Conclusion** (6.1–6.3) | **COMPLETE & VERIFIED** | Pages 35–36 | Honest conclusions: throughput vs tail latency trade-off; QLAAC p95 exclusion of queue wait; counter bug marked resolved; genuine limitations (Slowloris) and future work retained. |
| **7. References** (IEEE style) | **COMPLETE & VERIFIED** | Page 37 | All 11 citations verified with complete DOIs, volume, issue, and page numbers. |
| **Appendix A** (Key code & config listings) | **COMPLETE & VERIFIED** | Pages 38–39 | Code excerpts verified against exact final source files; A.3 updated to side-effect-free `adaptive_controller_get_state()` worker query and accept-gate shedding. |
| **Appendix B** (Screenshots, authentic test output, raw tables) | **COMPLETE & VERIFIED** | Pages 40–46 | Execution output documents 37 C unit tests and 35 Python tests (unit and integration suites, 72 total); AddressSanitizer, UBSan, and ThreadSanitizer results verified; raw tables and historical records complete. |

---

## 2. Issues Corrected Across Project Audits

1. **Team Particulars & Authorship Attribution:**
   - Corrected cover page Table 0:
     - Row 0: `24BCE0702` | `YASH PRADHAN`
     - Row 1: `24BCE0714` | `ANJINI PANDEY`
   - Corrected Certificate paragraph (P[17]):
     - "This is to certify that the project report titled 'Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server from Scratch Using Raw TCP Sockets' is a bonafide record of the work done by **YASH PRADHAN (24BCE0702)** and **ANJINI PANDEY (24BCE0714)** in partial fulfilment of the requirements of the Innovative Evaluation Practices for BCSE308P -Computer Networks Lab during the Fall Semester 2026-2027."
   - Removed all placeholders: `[Reg. No.]`, `[TEAM MEMBER – CONFIRM]`, and `[add remaining team members and Reg. Nos., sorted by Reg. No., if applicable]`.
   - Updated Word document core properties (`Author: Yash Pradhan, Anjini Pandey`).

2. **Resolution of Execution Environments & Toolchain Discrepancy (Section 4.1, Appendix B.2, Appendix B.6):**
   - **Resolved GCC 15.2.0 vs GCC 13.3.0 and Python 3.14.4 vs Python 3.13.16 Discrepancy:**
     - Verified actual test logs: Appendix B.2 contains a genuine test run executed on an Ubuntu 24.04 LTS containerized authoring sandbox on 17 October 2026 (`Linux 6.6.137+ x86_64`, GCC 13.3.0, Python 3.13.16).
     - The verification platform used for benchmark evaluation and workstation validation in Section 4.1 is an Intel(R) Core(TM) Ultra 7 155H host running WSL2 (`Linux 6.18.33.2-microsoft-standard-WSL2`, GCC 15.2.0, Python 3.14.4).
     - Table 4.1 (Table 6 in DOCX) was explicitly updated to document all three environments separately without conflation or invented details:
       1. *Student Development Platform:* Linux via WSL2 / Windows 11 host (POSIX C17 / GCC 11+).
       2. *Local Workstation Verification Platform:* Intel Core Ultra 7 155H (16 physical cores, 22 vCPUs, 7.5 GiB RAM, Linux 6.18 WSL2, GCC 15.2.0, Python 3.14.4).
       3. *Supplementary Authoring & Mechanism Sandbox:* Ubuntu 24.04 LTS container (2 logical CPUs, GCC 13.3.0, Python 3.13.16).
   - **Clarified 2-vCPU (Appendix B.6) vs 22-vCPU (Section 4.1) Resource Limits:**
     - Appendix B.6 documents a supplementary 5-second qualitative mechanism check ($C = 8$ and $C = 64$) executed in the 2-vCPU supplementary sandbox environment to demonstrate admission-controller state transitions under constrained CPU allocation.
     - Appendix B.2 and B.6 were explicitly annotated to identify the 2-vCPU containerized environment, preventing confusion with the primary 22-vCPU workstation platform.

3. **Trial Independence & QLAAC State Carryover Qualification (Section 4.2, Section 5.3, Section 5.5):**
   - Audited `benchmark/run_experiment.py`: the benchmark harness launches a single server process and executes consecutive 10-second trials sequentially across client concurrency tiers ($C = 2, 8, 32, 64$) with a 0.5 s sleep interval between trials.
   - The server was *not* restarted between consecutive trials within a run.
   - For the baseline server (static policy), trials are functionally independent of internal server state.
   - For QLAAC (adaptive controller with 1.0 s sampling epoch, 5-window hysteresis, and rolling latency history), internal controller state (e.g., hysteresis counter, smoothed latency, and shedding probability) can carry over between consecutive trials if the server is not restarted.
   - Replaced claims of "independent trials" with "repeated trials" throughout Section 4.2 (Table 7 & P[116], P[119]), Section 5.3 (P[144]), and Section 5.5 (P[176]).
   - Explicitly qualified sample standard deviations as reflecting repeated-trial empirical dispersion rather than independent identically distributed (i.i.d.) trial variance.

4. **Package README Benchmark Synchronization (`CLAUDE_PACKAGE_README.md`):**
   - Corrected outdated placeholder/preliminary figures in `CLAUDE_PACKAGE_README.md` (lines 40, 146–186):
     - Removed outdated $C=1$ row and unverified throughput numbers (1,215.8 RPS, 182.4 RPS, ~87.5% shedding, unbounded queue claims).
     - Synchronized all benchmark tables and discussion with the verified 20-trial empirical dataset ($C = 2, 8, 32, 64$; 6,828.52 baseline RPS vs 638.12 QLAAC RPS at $C=64$; $72.14\% \pm 5.15\%$ shedding rate; bounded queue capacity 64).
   - Rebuilt `deliverables/claude_report_package.zip` with updated files.

5. **Course-Module Mapping (Section 1.5):**
   - Verified that no official BCSE308P syllabus document is present in the repository.
   - Removed editorial bracket `[Student to add module numbers/titles from the BCSE308P syllabus...]`.
   - Formally stated that because official curriculum module numbers could not be verified from archived repository files, they were not fabricated. Retained the rigorous mapping in Table 1.1 connecting project components to foundational Computer Networks concepts.

6. **Queue Description & Architectural Invariants:**
   - Confirmed that the baseline server queue is strictly a thread-safe bounded circular FIFO with fixed capacity 64 (`connection_queue_capacity = 64`), guarded by a mutex and condition variables. It is nowhere described as unbounded.

7. **Literature & Citations (Section 7):**
   - Enriched all 11 IEEE citations with complete bibliographic records (SOSP 2001, USITS 2003, ACM TOIT 2006, EuroSys 2007, ACM Queue 2011/2012, CACM 2013, Operations Research 1961).

8. **Test Evidence & Visual Integrity:**
   - Acknowledged that `results/screenshots/06_test_results.png` was synthetically generated. Confirmed it is excluded as proof of test execution.
   - Verified actual textual execution logs citing authentic test suites.

9. **Table of Contents & Pagination:**
   - Updated the static TOC page numbers in the DOCX. Verified the principal section starts against the current 46-page PDF rendered from the DOCX using LibreOffice 25.2.3.2. Recheck pagination if a Microsoft Word export is used, as the layout engine may differ.

10. **Queue-Wait Formula Dimension Correction (Table 5.4 & Section 5.4):**
    - Corrected the predicted-wait formula header in Table 5.4 from `rounds × 200 / X (ms)` to `rounds × (200 / X) × 1000 ms`.
    - Explicitly clarified that because throughput $X$ is measured in requests/second, the round duration $(200 / X)$ evaluates in seconds; multiplying by $1000$ yields milliseconds.
    - Preserved all calculated values in Table 5.4 (30.2 ms, 204.1 ms, 439.3 ms) and updated Section 5.4 (P[160]) for complete formulation alignment.

11. **Historical Discrepancy Framing for Package README (Appendix B.4 & Section 5.3):**
    - Clarified in Section 5.3 (P[142]) and Appendix B.4 (P[324], P[325], Table B.4) that the quoted figures describe the pre-audit version of `CLAUDE_PACKAGE_README.md`, which has since been corrected to match empirical CSV data.
    - Preserved Table B.4 as a historical discrepancy record documenting what was previously stated versus the verified raw CSV data, rather than presenting it as a current error.

12. **Abstract Word Count:**
    - The current abstract has 297 whitespace-separated words, below the 300-word maximum using this counting convention. Microsoft Word's own word count has not been independently confirmed in this pass and remains a final check.

13. **Final Consistency, Technical Accuracy, and Document-Quality Pass:**
    - **Exact Test Suite Breakdown (72 automated tests total):**
      - *C Unit Tests (37 total, `make test`):* Controller: 9, Parser: 14, Queue: 3, Responses: 4, Server Metrics: 7. All 37 executed and passed.
      - *Python Test Suite (35 total across unit and integration suites):* Benchmark Accounting: 12 (offline unit), Keep-Alive: 6 (2 unit / 4 integration), Malformed Requests: 5 (integration), Security: 12 (integration). All 35 executed and passed against active server daemon on port 8080 (0 failures, 0 skipped). When server is absent, 21 server-dependent integration tests skip gracefully while 14 standalone unit tests pass.
      - Updated every current-state occurrence in the Abstract, Section 1.2, Section 4.3, Section 6.1, Appendix B.2, and checklist from outdated 49 (26 C, 23 Python) to verified 72 (37 C, 35 Python).
    - **QLAAC Counter Bug Resolution & Metric Semantics:**
      - Replaced obsolete snippet and explanation in Appendix A.3: worker threads now query controller state via `adaptive_controller_get_state()` (pure, idempotent, side-effect-free query without state modification or counter increments).
      - Documented distinct, precise server-side counter semantics:
        1. `qlaac_shed_requests`: counts QLAAC admission-rejection decisions made by the controller at the accept gate.
        2. `total_connections_rejected`: counts all connection-rejection paths, combining QLAAC shed decisions and bounded-queue-full drops.
        3. `total_overload_503_sent`: counts successful server-side writes of HTTP 503 responses across all rejection paths.
      - Clarified that because queue-full rejections add successful 503 writes without incrementing `qlaac_shed_requests`, while client connections that close or reset before or during the server's HTTP 503 response write prevent `total_overload_503_sent` from incrementing, there is no guaranteed ordering between the QLAAC shed counter and the successful-503-write counter.
      - Updated Table 4.4, Section 5.5, Section 6.1, Section 6.2, and Section 6.3 to present the bug as resolved rather than an unresolved defect or future work.
      - Preserved Appendix B.6's observed counter discrepancy (29,783 shed vs 26,978 503s) with explicit qualification that it reflects the pre-fix implementation.
    - **Confirmed Security & Implementation Fixes (Table 4.4 & Section 4.3):**
      - Segment-aware path traversal parser (allowing benign filenames like `release..txt`).
      - URL percent-decoding with strict rejection of malformed escapes, NUL bytes (%00), and backslash paths.
      - Canonical root validation with `realpath()` and symlink protections (`O_NOFOLLOW` / `fstat` device/inode validation).
      - Strict RFC 9110/9112 `1*DIGIT` Content-Length grammar (rejecting negative signs, non-digits, and overflow to 413/400).
      - Strict header parsing rejecting bare CR (RFC 9112 §2.2) and bare LF (defensive server choice; RFC 9112 §5.2 covers obsolete line folding); individual names of 128 bytes or more and values of 2,048 bytes or more are rejected with 431 per RFC 9110 §5.4 (Field Limits). Total header block limit is `max_header_size`: 8,192 bytes when the example config loads, otherwise 16,384 bytes with compiled defaults; more than 64 header fields is also rejected.
      - Full response cleanup preventing heap buffer and file descriptor leaks on error paths.
      - Zero-copy `sendfile(2)` fallback loop with dynamic `lseek(SEEK_CUR)` offset tracking.
      - Signal-safe shutdown notification (`volatile sig_atomic_t`) and prompt termination of idle keep-alive workers (`atomic_bool stop_requested` / `worker_idle_timeout_sec`).
      - Replaced outdated claim in Section 6.2 that no security audit was performed with precise description of targeted internal audit, while retaining genuine architectural limitations (slow-client/Slowloris starvation).
    - **Sanitizer & Static Analysis Verification:**
      - Rebuilt and executed full C unit test suite (5 executables, 37 tests) under AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan): 37/37 passed, 0 memory errors, 0 leaks, 0 undefined behaviors.
      - Rebuilt and executed full C unit test suite (5 executables, 37 tests) under ThreadSanitizer (TSan): 37/37 passed, 0 data races (coverage covers unit/subsystem test harnesses; long-running server daemon execution was not instrumented under TSan). Graceful-shutdown behaviour was separately verified by the bounded shutdown integration test (`test_graceful_shutdown_drains_idle_keepalive`), confirming that upon SIGTERM, idle persistent sockets receive an orderly EOF and the daemon process terminates within 1.5 seconds rather than blocking for the 5-second keepalive timeout.
      - Executed GCC static analysis (`gcc -fanalyzer -std=c17`) across all 12 source files in `src/`: 0 warnings, 0 diagnostic defects.
    - **Configuration Alignment & Resolution:**
      - Checked `config/server.conf.example` and restored baseline default `adaptive_control_enabled = false`.
      - Verified command-line precedence: `./bin/server -p 8089` starts with `qlaac_enabled: false`; `./bin/server -p 8089 -a` overrides to `qlaac_enabled: true`. Verified live via `/metrics` JSON endpoint.
      - Resolved `worker_idle_timeout_sec`: added field to `ServerConfig`, parsed in `server_config_load_file`, and passed dynamically to `connection_queue_pop` in worker loop.
    - **Source Line Counts Recalculation (Table 4.3):**
      - Recalculated from final files: `src/*.c` (3,405 lines across 12 files), `include/*.h` (1,108 lines across 11 files), C unit tests (1,222 lines), Python tests (838 lines). Table 4.3 synchronized.
    - **Benchmark Chronology & Dataset Preservation:**
      - Confirmed primary empirical CSV datasets in `results/raw/` (`experiment_baseline_tuned_comparison.csv` and `experiment_qlaac_tuned_comparison.csv`) were preserved intact.
      - Explicitly documented that Chapter 5 results reflect the original empirical benchmark runs, while subsequent code-audit fixes were verified through the 72 targeted functional and regression tests without re-running heavy multi-client load.
    - **Current PDF Export & Static TOC Synchronization:**
      - Rendered `CN_Project_Final_Report.pdf` from the corrected DOCX using LibreOffice 25.2.3.2; output has 46 pages.
      - The static TOC entries align with the current generated PDF for the principal chapter and appendix starts. Microsoft Word word count and Word-native PDF pagination remain unverified.
      - The benchmark launch working directory was not recorded, so the effective configured header-block limit at benchmark time (8,192 or 16,384 bytes) remains unverified; requests in the workload were small GET `/health` requests.

14. **Header-Size, Buffer-Configuration, and URI-Boundary Audit:**
    - `main.c` initializes configuration defaults and attempts to read `config/server.conf.example` by default (or the `-c`/`--config` path) relative to the working directory. The example file sets `max_header_size = 8192` and `io_buffer_size = 8192`; compiled defaults are 16384. If the file cannot be read, compiled defaults remain active.
    - `thread_pool.c` passes `config.max_header_size` to the parser; the parser rejects a total header block larger than that limit with 431.
    - Compile-time field limits are separate: header names of 128 bytes or more and values of 2,048 bytes or more receive 431; no more than 64 fields are accepted. The longest accepted individual name/value lengths are 127/2,047 bytes. `DEFAULT_MAX_HEADER_SZ` in the header is unused by `http_parser.c`.
    - `http_parser.c` rejects URI lengths of 2,048 bytes or more with 414; the longest accepted URI is 2,047 bytes. Table 4.3 has been corrected to `URI ≤ 2,047 B`.
    - The Chapter 5 launch working directory is not recorded; the effective total header limit in those runs remains unverified. The workload used small GET `/health` requests, but any claim that this made the limit immaterial is an inference, not recorded evidence.

---

## 3. Numerical Verification Summary

All values in Chapter 5 and Appendix B match `benchmark/verify_benchmark_statistics.py` directly computed from `results/raw/experiment_baseline_tuned_comparison.csv` and `results/raw/experiment_qlaac_tuned_comparison.csv` ($n = 5$ repeated 10-second trials, sample standard deviation):

| Clients ($C$) | Configuration | Verified 2xx RPS (Mean $\pm$ SD) | $p50$ Latency (ms) | $p95$ Latency (ms) | $p99$ Latency (ms) | HTTP 5xx Sent (Mean $\pm$ SD) | Shedding Rate (%) |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **2** | Baseline | 13,929.24 $\pm$ 334.23 | 0.13 $\pm$ 0.01 | 0.23 $\pm$ 0.01 | 0.31 $\pm$ 0.02 | 0.00 $\pm$ 0.00 | 0.00% $\pm$ 0.00% |
| **2** | QLAAC | 14,665.40 $\pm$ 610.30 | 0.12 $\pm$ 0.00 | 0.22 $\pm$ 0.03 | 0.33 $\pm$ 0.05 | 0.00 $\pm$ 0.00 | 0.00% $\pm$ 0.00% |
| **8** | Baseline | 6,629.38 $\pm$ 148.21 | 0.54 $\pm$ 0.01 | 1.38 $\pm$ 0.04 | 29.09 $\pm$ 0.50 | 0.00 $\pm$ 0.00 | 0.00% $\pm$ 0.00% |
| **8** | QLAAC | 2,129.26 $\pm$ 50.35 | 1.55 $\pm$ 0.02 | 2.83 $\pm$ 0.03 | 3.55 $\pm$ 0.06 | 0.00 $\pm$ 0.00 | 0.00% $\pm$ 0.00% |
| **32** | Baseline | 6,860.38 $\pm$ 107.11 | 0.52 $\pm$ 0.01 | 1.34 $\pm$ 0.03 | 204.00 $\pm$ 2.94 | 0.00 $\pm$ 0.00 | 0.00% $\pm$ 0.00% |
| **32** | QLAAC | 1,970.40 $\pm$ 55.83 | 10.46 $\pm$ 0.26 | 14.31 $\pm$ 0.66 | 16.55 $\pm$ 1.00 | 0.00 $\pm$ 0.00 | 0.00% $\pm$ 0.00% |
| **64** | Baseline | 6,828.52 $\pm$ 157.91 | 0.52 $\pm$ 0.01 | 1.38 $\pm$ 0.05 | 440.33 $\pm$ 9.21 | 0.00 $\pm$ 0.00 | 0.00% $\pm$ 0.00% |
| **64** | QLAAC | 638.12 $\pm$ 126.44 | 19.61 $\pm$ 6.35 | 28.76 $\pm$ 0.59 | 161.18 $\pm$ 159.88 | 16,475.40 $\pm$ 969.70 | 72.14% $\pm$ 5.15% |

**Key Findings Faithfully Preserved:**
- At $C = 2$, measured results were similar (p99 0.31 vs 0.33 ms); exploratory tests are interpreted cautiously because trials were sequential and QLAAC controller state may carry over.
- At $C = 8$ and $C = 32$, the measured trials showed large reductions in successful-response $p99$ latency (3.55 ms vs 29.09 ms and 16.55 ms vs 204.00 ms, respectively). At $C = 64$, p99 fell from 440.33 ms to 161.18 ms on average but varied widely between trials ($161.18 \pm 159.88$ ms); the throughput and rejection trade-offs must be considered.
- The cost of tail-latency capping is substantial throughput degradation: successful 2xx throughput drops by 67.9% ($C = 8$), 71.3% ($C = 32$), and 90.7% ($C = 64$).
- At $C = 64$, 72.14% of completed requests are HTTP 503 overload rejections, and $p99$ latency exhibits high sample standard deviation ($159.88$ ms, ~99% of the mean), reflecting cyclic oscillation between OVERLOAD and CAUTION recovery.

---

## 4. Visual Inspection Checklist

- [x] **Cover Page:** VIT logo intact, title properly formatted, team members listed in ascending registration number order (24BCE0702 Yash Pradhan, 24BCE0714 Anjini Pandey).
- [x] **Certificate:** Clean academic wording, student names and registration numbers formatted in bold, no placeholders.
- [ ] **Abstract:** 297 whitespace-separated words in the current DOCX (under the 300-word limit by this count); confirm the final count with Microsoft Word. The test count statement is 72 tests: 37 C, 35 Python.
- [x] **Table of Contents:** Spans Pages 4–5; static numbers checked against the current 46-page LibreOffice PDF. [ ] Recheck after any Microsoft Word export.
- [x] **Typography & Margins:** Times New Roman body text (12 pt), section headers (14 pt bold), consistent table cell padding, no clipped images or overflowing tables.
- [x] **Diagrams & Flowcharts:** Figures 3.1, 3.2, 3.3, 5.1, 5.2, 5.3, 5.4, 5.5, B.1, B.2, B.3, B.4 readable and within page boundaries.
- [x] **Table 4.3:** Source module line counts recalculated from verified final `wc -l` outputs.
- [x] **Table 4.4:** Challenges and solutions table expanded with confirmed security, HTTP framing, sendfile offset fallback, and QLAAC counter reconciliation evidence.
- [x] **Table 5.4:** Column header displays `Predicted wait = rounds × (200 / X) × 1000 ms`, correctly accounting for throughput in req/s with calculated values preserved (30.2, 204.1, 439.3).
- [x] **Table B.4:** Accurately framed as a historical discrepancy record of pre-audit package README claims compared with raw CSV files.
- [x] **Appendix A & B:** Cleanly formatted without clipped code excerpts or broken tables; A.3 updated to side-effect-free state query; Appendix B.2 contains verified 37 C / 35 Python test execution logs.

---

## 5. Remaining Finalization Checks

- **Microsoft Word abstract count:** Confirm the abstract count directly in Word; the current abstract contains 297 whitespace-separated words, but Word's own count may handle hyphenated terms differently.
- **Microsoft Word PDF pagination:** The attached PDF was generated from the corrected DOCX with LibreOffice 25.2.3.2 (46 pages). If a Microsoft Word-exported PDF is required, export it from Word and recheck the static TOC against that version before submission.
- **Benchmark launch directory:** The working directory during Chapter 5 benchmarks was not recorded; therefore, whether `config/server.conf.example` loaded and set the total header limit to 8,192 bytes, rather than the 16,384-byte compiled default, is **UNVERIFIED**. No benchmark results were changed.
- **URI limit:** Source inspection confirms that URI lengths of 2,048 bytes or more are rejected with 414, so the maximum accepted URI length is 2,047 bytes. Table 4.3 now states `URI ≤ 2,047 B`; the earlier discrepancy is resolved.
- **Optional curriculum mapping:** If the official BCSE308P syllabus becomes available, the general concept mapping in Section 1.5 can optionally be expanded with official module numbers. No curriculum modules have been invented.
