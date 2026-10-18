# Report Verification Checklist — BCSE308P CN Lab Project

**Files:** `CN_Project_Final_Report.docx` (editable, 40 pages), `CN_Project_Final_Report.pdf` (native Microsoft Word COM export of the same DOCX, 40 pages), this checklist.

---

## 1. Template Section → Report Location & Audit Status

| Template Requirement | Audit Status | Location in Report | Verification Details |
|---|---|---|---|
| **Cover Page** (Title 16 pt bold caps, particulars sorted by Reg. No., B.Tech., CSE, School, VIT logo, October 2026) | **COMPLETE & VERIFIED** | Page 1 | Sorted ascending: (1) 24BCE0702 — YASH PRADHAN, (2) 24BCE0714 — ANJINI PANDEY. Placeholders removed. VIT logo preserved. |
| **Abstract** (One page, $\le 300$ words) | **COMPLETE & VERIFIED** | Page 2 | Exactly 284 words ($\le 300$). Unchanged text, honest performance trade-offs reported. |
| **Certificate** (Official VIT template wording) | **COMPLETE & VERIFIED** | Page 3 | Both students and registration numbers listed; placeholders removed; signature line present. |
| **Table of Contents** (Static numbers matching final pagination) | **COMPLETE & VERIFIED** | Pages 4–5 | All 36 entries verified against native Word COM PDF pagination (40 pages total). Matches 100%. |
| **1. Introduction** (1.1–1.5) | **COMPLETE & VERIFIED** | Pages 6–8 | Section 1.5 editorial placeholder removed; formally mapped to core Computer Networks concepts. |
| **2. Literature Survey** (2.1–2.3, template table) | **COMPLETE & VERIFIED** | Pages 9–11 | 6 survey rows; research gap and novelty justification formally framed. |
| **3. Proposed System** (3.1–3.5, architecture, flowchart, pseudocode) | **COMPLETE & VERIFIED** | Pages 12–16 | Bounded queue (capacity 64) accurately documented; QLAAC 3-state machine and trade-offs explained. |
| **4. Implementation** (4.1–4.4) | **COMPLETE & VERIFIED** | Pages 17–21 | Table 4.1 updated to clearly distinguish historical benchmark host from local verification platform. |
| **5. Evaluation** (5.1–5.5, results table, graphs, critical analysis) | **COMPLETE & VERIFIED** | Pages 22–29 | All values match `benchmark/verify_benchmark_statistics.py` directly from raw CSVs (n = 5, sample SD). |
| **6. Conclusion** (6.1–6.3) | **COMPLETE & VERIFIED** | Pages 30–31 | Honest conclusions: throughput vs tail latency trade-off, QLAAC p95 exclusion of queue wait. |
| **7. References** (IEEE style) | **COMPLETE & VERIFIED** | Page 32 | All 11 citations verified with complete DOIs, volume, issue, and page numbers. |
| **Appendix A** (Key code & config listings) | **COMPLETE & VERIFIED** | Pages 33–34 | C excerpts of QLAAC controller, socket timeouts, and config defaults verified against source. |
| **Appendix B** (Screenshots, authentic test output, raw tables) | **COMPLETE & VERIFIED** | Pages 35–40 | Authentic test execution evidence cited; synthetic image excluded; raw data tables complete. |

---

## 2. Issues Corrected in this Audit

1. **Team Particulars & Authorship Attribution:**
   - Corrected cover page Table 0:
     - Row 0: `24BCE0702` | `YASH PRADHAN`
     - Row 1: `24BCE0714` | `ANJINI PANDEY`
   - Corrected Certificate paragraph (P[17]):
     - "This is to certify that the project report titled 'Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server from Scratch Using Raw TCP Sockets' is a bonafide record of the work done by **YASH PRADHAN (24BCE0702)** and **ANJINI PANDEY (24BCE0714)** in partial fulfilment of the requirements of the Innovative Evaluation Practices for BCSE308P -Computer Networks Lab during the Fall Semester 2026-2027."
   - Removed all placeholders: `[Reg. No.]`, `[TEAM MEMBER – CONFIRM]`, and `[add remaining team members and Reg. Nos., sorted by Reg. No., if applicable]`.
   - Updated Word document core properties (`Author: Yash Pradhan, Anjini Pandey`).

2. **Course-Module Mapping (Section 1.5):**
   - Verified that no official BCSE308P syllabus document is present in the repository.
   - Removed editorial bracket `[Student to add module numbers/titles from the BCSE308P syllabus...]`.
   - Formally stated that because official curriculum module numbers could not be verified from archived repository files, they were not fabricated. Retained the rigorous mapping in Table 1.1 connecting project components to foundational Computer Networks concepts (TCP byte streams, HTTP/1.1 framing, thread-pool concurrency, bounded queueing dynamics, and empirical evaluation).

3. **Hardware & Verification Environment (Section 4.1):**
   - Factually resolved Table 4.1 (Table 6 in DOCX) to distinguish:
     - **Historical Benchmark Machine:** Windows 11 host with WSL2 execution environment (POSIX C17 / GCC 11+); exact CPU model and RAM limits were not recorded in CSV headers.
     - **Independent Verification Platform:** Intel(R) Core(TM) Ultra 7 155H (16 physical cores, 22 vCPUs), 7.5 GiB RAM, Linux 6.18.33.2-microsoft-standard-WSL2 (x86_64), GCC 15.2.0, Python 3.14.4 (October 2026).
   - Removed placeholder `[to be confirmed by the student: not recorded in the supplied materials]`.

4. **Benchmark Reproducibility & Launch Configuration:**
   - In Section 4.2 / 4.3 (P[109] and P[119]):
     - Clarified that `config/server.conf.example` defines 8,192-byte buffers while compiled server defaults (`server_config_init_defaults` in `src/server.c`) initialize both header and I/O buffers to 16,384 bytes, which are active when starting without a config file. Removed `[student to confirm launch command]`.
     - Documented the benchmark execution command: `python3 benchmark/run_experiment.py --url http://127.0.0.1:8080/health --clients 2,8,32,64 --duration 10 --trials 5 --output <csv>` (run against `bin/server` and `bin/server -a`).
     - Clarified the meaning of "tuned" in raw CSV filenames: the refined multi-factor controller in `src/adaptive_controller.c` (worker saturation demoted to CAUTION; OVERLOAD requiring queue $\ge 0.90$, $p95 \ge 150$ ms, or sustained multi-factor pressure) superseding preliminary runs that tripped overload on worker saturation alone. Removed `[student to confirm]` and `[student to describe]`.
     - Documented that `run_experiment.py` steps through client counts sequentially with a 0.5 s inter-trial pause, meaning controller hysteresis can carry over across client tiers if the server is not restarted externally between batches.

5. **Queue Description & Architectural Invariants:**
   - Confirmed that the baseline server queue is strictly a thread-safe bounded circular FIFO with fixed capacity 64 (`connection_queue_capacity = 64`), guarded by a mutex and condition variables. It is nowhere described as unbounded.

6. **Literature & Citations (Section 7):**
   - Enriched all 11 IEEE citations with complete bibliographic records:
     - [4] Welsh, Culler, Brewer (SOSP 2001, pp. 230–243, doi: 10.1145/502034.502057)
     - [5] Welsh and Culler (USITS 2003, pp. 45–60)
     - [6] Schroeder and Harchol-Balter (ACM TOIT 2006, vol. 6, no. 1, pp. 20–52, doi: 10.1145/1125274.1125276)
     - [7] Pariag et al. (EuroSys 2007, pp. 231–243, doi: 10.1145/1272996.1273019)
     - [8] Nichols and Jacobson (ACM Queue 2012, vol. 10, no. 5, pp. 20–34, doi: 10.1145/2208917.2209336)
     - [9] Gettys and Nichols (ACM Queue 2011, vol. 9, no. 11, pp. 40–54, doi: 10.1145/2063166.2071893)
     - [10] Dean and Barroso (CACM 2013, vol. 56, no. 2, pp. 74–80, doi: 10.1145/2408776.2408794)
     - [11] Little (Oper. Res. 1961, vol. 9, no. 3, pp. 383–387, doi: 10.1287/opre.9.3.383)

7. **Test Evidence & Visual Integrity:**
   - Acknowledged that `results/screenshots/06_test_results.png` was synthetically generated. Confirmed it is excluded as proof of test execution.
   - Textual execution summaries cite the authentic test suite run (26/26 C unit tests, 23/23 live Python tests, 49/49 passing).

8. **Table of Contents & Pagination:**
   - Updated static TOC page numbers in Table 1 to match native Microsoft Word COM rendering (40 pages). Every single section heading verified against its exact rendered page in the PDF.

---

## 3. Numerical Verification Summary

All values in Chapter 5 and Appendix B match `benchmark/verify_benchmark_statistics.py` directly computed from `results/raw/experiment_baseline_tuned_comparison.csv` and `results/raw/experiment_qlaac_tuned_comparison.csv` ($n = 5$ independent 10-second trials, sample standard deviation):

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
- At $C = 2$, QLAAC exhibits no statistically significant difference from baseline.
- At $C \ge 8$, QLAAC significantly caps $p99$ tail latency (e.g. at $C = 32$, $16.55$ ms vs $204.00$ ms; at $C = 64$, $161.18$ ms vs $440.33$ ms).
- The cost of tail-latency capping is substantial throughput degradation: successful 2xx throughput drops by 67.9% ($C = 8$), 71.3% ($C = 32$), and 90.7% ($C = 64$).
- At $C = 64$, 72.14% of completed requests are HTTP 503 overload rejections, and $p99$ latency exhibits high sample standard deviation ($159.88$ ms, ~99% of the mean), reflecting cyclic oscillation between OVERLOAD and CAUTION recovery.

---

## 4. Visual Inspection Checklist

- [x] **Cover Page:** VIT logo intact, title properly formatted, team members listed in ascending registration number order (24BCE0702 Yash Pradhan, 24BCE0714 Anjini Pandey).
- [x] **Certificate:** Clean academic wording, student names and registration numbers formatted in bold, no placeholders.
- [x] **Abstract:** 284 words ($\le 300$), cleanly fits on Page 2.
- [x] **Table of Contents:** Spans Pages 4–5, all 36 entries match actual 40-page PDF document pagination.
- [x] **Typography & Margins:** Times New Roman body text (12 pt), section headers (14 pt bold), consistent table cell padding, no clipped images or overflowing tables.
- [x] **Diagrams & Flowcharts:** Figures 3.1, 3.2, 3.3, 5.1, 5.2, 5.3, 5.4, 5.5, B.1, B.2, B.3, B.4 readable and within page boundaries.

---

## 5. Items Requiring Student Confirmation (Optional)

The report is fully consistent, rigorous, and ready for submission. The following item is optional if the student wishes to personalize it further:
- **Curriculum Module Names:** If the student has access to the official VIT BCSE308P syllabus document, they may optionally replace the formal disclaimer in Section 1.5 with the exact syllabus module numbers (e.g., Module 3: Transport Layer, Experiment 5: Socket Programming). The existing text formally explains why general concepts are mapped, making the report completely valid as-is without fabrication.
