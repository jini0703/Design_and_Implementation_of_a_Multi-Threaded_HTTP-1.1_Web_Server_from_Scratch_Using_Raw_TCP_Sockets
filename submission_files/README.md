# Final Academic Project Deliverables — BCSE308P Team 17

This directory is the dedicated location for the official evaluated academic deliverables for the Computer Networks laboratory course:

**Project Title:** *Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server from Scratch Using Raw TCP Sockets*
**Course:** Innovative Evaluation Practices for BCSE308P — Computer Networks Lab
**Academic Term:** Fall Semester 2026–27
**School:** School of Computer Science and Engineering, Vellore Institute of Technology

### Official Course Identifiers
- **Slot:** `F1+TF1`
- **Team:** `Team 17`

### Project Team
- **24BCE0702 — Yash Pradhan**
- **24BCE0714 — Anjini Pandey**

---

## VTOP Assignment Deliverables

Per course guidelines, the canonical submission files follow the prescribed slot and team naming convention:

| VTOP Component | Canonical Filename | Format / Size | Status & Evaluation Description |
| :--- | :--- | :---: | :--- |
| **Assignment 8: Project Report** | [`F1+TF1_Team17_Report.pdf`](F1+TF1_Team17_Report.pdf) | PDF (54 pages)<br>2,632,172 B | **Definitive Final Report**: Complete 54-page project report formatted to official VIT template, with full empirical benchmark datasets, 11 IEEE references, and verification appendices. |
| **Assignment 9: Presentation** | [`F1+TF1_Team17_PPT.pptx`](F1+TF1_Team17_PPT.pptx) | PPTX (29 slides)<br>2,171,208 B | **Definitive Oral Defense Deck**: 29 widescreen slides structured across the teacher's ten mandatory evaluation topics. |
| **Assignment 9 Supporting Preview** | [`F1+TF1_Team17_PPT.pdf`](F1+TF1_Team17_PPT.pdf) | PDF (29 pages)<br>1,087,199 B | **Matching PDF Preview**: Direct 1-to-1 slide-for-slide rendering of the canonical 29-slide PowerPoint deck. |
| **Supporting Report Source** | [`F1+TF1_Team17_Report.docx`](F1+TF1_Team17_Report.docx) | DOCX (Editable)<br>2,899,989 B | **Formatted Word Source**: Preserves full styling, static table of contents, and all 24 data tables. |
| **Audit Checklist** | [`Report_Verification_Checklist.md`](Report_Verification_Checklist.md) | Markdown | **Verification Audit**: Section-by-section audit verifying abstract word count ($\le 300$), pagination, 92 test cases, and empirical consistency. |
| **Assignment 10: GitHub Repository** | [GitHub Repository](https://github.com/jini0703/Design_and_Implementation_of_a_Multi-Threaded_HTTP-1.1_Web_Server_from_Scratch_Using_Raw_TCP_Sockets) | Git Repository | **Authoritative Project Tree**: Complete C17 source code, headers, Makefile, tests, benchmark harness, and documentation on branch `main`. |

### Optional External Supporting Archives (for VTOP upload fields if required)
If standalone archives are requested by VTOP upload fields, the following matching archives are prepared outside the Git repository:
- `F1+TF1_Team17_SourceCode.zip` — Standalone clean archive of source code, configurations, tests, and documentation.
- `F1+TF1_Team17_Screenshots.zip` — Standalone archive containing all five authentic runtime screenshots and high-resolution performance plots.

---

## Presentation Structure: 10 Evaluated Topics Across 29 Slides

The presentation strictly follows the ten required topics established by course faculty for project evaluation:

| Section | Topic | Slides | Content & Empirical Scope |
| :---: | :--- | :---: | :--- |
| **1** | **Title and Team** | Slide 1 | Course details (BCSE308P, Fall 2026–27), Slot `F1+TF1`, `Team 17`, student names & registration numbers (24BCE0702 Yash Pradhan, 24BCE0714 Anjini Pandey), project title. |
| **2** | **Problem and Motivation** | Slides 2–4 | Queueing delay dominating tail latency ($L = \lambda W$), bufferbloat under burst concurrency, research questions, networking foundations, and literature context. |
| **3** | **Existing Approach (Baseline) & Gap** | Slides 5–6 | Fixed pthread worker pool with bounded FIFO queue, queue-wait accumulation, throughput vs. tail-latency divergence at $C = 32$ and $C = 64$. |
| **4** | **Proposed Novelty** | Slide 7 | Closed-loop Queue-and-Latency-Aware Admission Control (QLAAC) introducing dynamic multi-signal overload detection and early request shedding. |
| **5** | **Architecture & Algorithm** | Slides 8–10 | End-to-end request-handling and feedback architecture, 3-state controller machine (`NORMAL`, `CAUTION`, `OVERLOAD`), threshold hysteresis, C17 source implementation. |
| **6** | **Implementation Setup** | Slides 11–18 | Raw POSIX TCP sockets (`AF_INET`, `SOCK_STREAM`), zero-copy `sendfile(2)`, circular queue synchronization, streaming RFC 9112 parser, sandbox security (`realpath`), build setup, challenges solved, genuine live runtime captures (Dashboard, `/health`, `/api/echo`, bursts). |
| **7** | **Scenarios and Metrics** | Slides 19–20 | Experimental methodology, concurrency tiers ($C = 2, 8, 32, 64$), paired trial protocol, 2xx vs. 5xx status accounting, joint throughput/tail-latency interpretation. |
| **8** | **Results (Baseline vs Proposed)** | Slides 21–24 | Historical v4 results at low/medium load, $C = 32$ CAUTION trade-offs, $C = 64$ OVERLOAD analysis (72.14% load shed, p99 capped), and separate verification-host rerun results. |
| **9** | **Analysis & Limitations** | Slides 25–27 | p95 vs. p99 rotation, timeline trajectories, failure containment, protocol & testbed limitations (Slowloris, synchronous worker I/O), verification chronology and test validation (92 automated tests). |
| **10** | **Conclusion, Future Work & References** | Slides 28–29 | Quantified findings, architectural lessons, future avenues (`io_uring`, HTTP/2), and complete IEEE references [1]–[11]. |

---

## Verification & Integrity Highlights

- **Report Length & Structure:** Exactly 54 pages in `F1+TF1_Team17_Report.pdf`, matching the official VIT template. Static table of contents aligns with every section and appendix page.
- **Abstract Word Count:** 275 whitespace-separated words (277 by Microsoft Word native counter), strictly within the academic $\le 300$-word limit.
- **Presentation Deck:** Exactly 29 slides in `F1+TF1_Team17_PPT.pptx` (16:9 widescreen) and 29 pages in `F1+TF1_Team17_PPT.pdf`.
- **Benchmark Integrity:** All empirical figures ($C = 2, 8, 32, 64$) stem directly from raw CSV records in `../results/raw/` without fabrication.
- **Historical Milestone Archive:** Early preliminary report drafting materials are preserved in [`../docs/archive/`](../docs/archive/) for provenance.
