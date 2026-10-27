# Project Presentation Deliverable — BCSE308P

## Overview

This directory documents the academic slide deck for the final oral defense and project presentation of the Computer Networks laboratory project:

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

## Canonical Submission Artifacts

Per teacher requirements for **VTOP Assignment 9**, the presentation must be submitted in PPT/PPTX format. The canonical files are located under [`deliverables/final_submission/`](../final_submission/):

1. **Primary Submission Deck (PPTX — 29 Slides):**
   [`deliverables/final_submission/F1+TF1_Team17_PPT.pptx`](../final_submission/F1+TF1_Team17_PPT.pptx)
   *Verified 29-slide widescreen deck following the teacher's ten mandatory evaluation sections.*
2. **Matching Presentation Preview (PDF — 29 Pages):**
   [`deliverables/final_submission/F1+TF1_Team17_PPT.pdf`](../final_submission/F1+TF1_Team17_PPT.pdf)
   *Exact 1-to-1 page export rendered from the canonical 29-slide PowerPoint deck.*

---

## Structure: 10 Evaluated Topics Across 29 Slides

The presentation strictly adheres to the ten required topics established by course faculty for project evaluation:

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

## Deliverable Integrity Verification

- **Slide Count:** Exactly 29 slides in `F1+TF1_Team17_PPT.pptx`.
- **Page Count:** Exactly 29 pages in `F1+TF1_Team17_PPT.pdf`.
- **Technical Parity:** Empirical metrics, tables, and architectural parameters match the 54-page canonical final report (`F1+TF1_Team17_Report.pdf`) byte-for-byte and claim-for-claim.
- **No Older Decks:** Historical drafts (e.g. earlier 18-slide or intermediate revision decks) have been superseded by this verified 29-slide canonical submission.
