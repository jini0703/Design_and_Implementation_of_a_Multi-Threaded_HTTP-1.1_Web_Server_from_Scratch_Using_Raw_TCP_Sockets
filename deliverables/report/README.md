# Project Report Deliverable

## Overview

This directory is the dedicated location for the comprehensive final academic technical report for the Computer Networks project:

**Project Title**: *Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server from Scratch Using Raw TCP Sockets*

### Expected Deliverable Filename

```text
HTTP11_Web_Server_Project_Report.pdf
```

---

## Status & Deliverable Verification

> **Verification Status: COMPLETED & VERIFIED**
> The comprehensive 12-page academic technical report has been compiled and verified in this directory:
> - **Final PDF Report**: [`HTTP11_Web_Server_Project_Report.pdf`](HTTP11_Web_Server_Project_Report.pdf) (12 pages, 1.85 MB, 6 embedded high-resolution figures)
> - **Source Markdown**: [`HTTP11_Web_Server_Project_Report.md`](HTTP11_Web_Server_Project_Report.md)
> - **Build Generator**: [`generate_report.py`](generate_report.py)
>
> All 21 IEEE/ACM formatted sections, empirical benchmark data tables from 40 controlled trials, architecture diagrams, and mathematical models are fully integrated. No placeholder content.

---

## Proposed Report Structure

The final report will be formatted according to standard IEEE/ACM systems-engineering formatting guidelines and will cover the following 21 structured sections:

1. **Title Page**: Formal course details, project title, author name, affiliation, and submission date.
2. **Abstract**: High-level synthesis of problem, architecture, from-scratch implementation, experimental methodology, and empirical conclusions.
3. **Introduction**: Motivation for systems-level networking, role of HTTP/1.1 in modern computing, and project overview.
4. **Problem Statement**: Challenges of building a robust web server directly on POSIX byte streams (framing, partial I/O, concurrency races, resource bounds).
5. **Objectives and Scope**: Concrete milestones, functional scope (C17, raw sockets, pthreads), and explicit boundaries.
6. **Background and Related Work**: Historical evolution of HTTP/1.0 through HTTP/1.1 (RFC 9112), comparison of threading models (per-connection, event-driven, thread-pool).
7. **Computer Networks Concepts**: TCP transport semantics, sliding window, flow control, connection handshake, teardown (FIN/RST), and socket state diagrams.
8. **TCP Socket Architecture**: Socket creation, address binding, backlog mechanics, timeout configurations (`SO_RCVTIMEO`, `SO_SNDTIMEO`), and partial read/write loops.
9. **HTTP/1.1 Protocol Implementation**: RFC 9112 parsing engine, request-line grammar, header extraction, case-insensitivity, persistent connections (`Connection: keep-alive`), and message framing via `Content-Length`.
10. **Multithreaded Server Design**: Architecture of the fixed-size pthread worker pool, thread dispatching, and elimination of process-fork overhead.
11. **Connection Queue and Synchronization**: Design of the thread-safe bounded FIFO circular buffer, mutex synchronization, condition variable signaling, and bounded capacity enforcement.
12. **Proposed QLAAC Admission-Control Mechanism**: Design of the experimental Queue-and-Latency-Aware Admission Control state machine (NORMAL, CAUTION, OVERLOAD), sampling windows, threshold triggers, and hysteresis dampening.
13. **Security and Reliability**: Threat model, path traversal defense via canonicalization, request size bounding, slowloris mitigation, descriptor cleanup invariants, and signal-safe shutdown.
14. **Implementation Details**: C17 code organization, memory management strategies, modular separation, and Linux kernel interfaces (`sendfile(2)`).
15. **Testing Methodology**: Automated unit testing in C, protocol verification in Python, malformed request fuzzing, and security penetration test suites.
16. **Benchmarking Setup**: Hardware environment specifications, kernel parameters, load generator tooling (`wrk`), and test matrix parameters.
17. **Experimental Results**: Presentation of verified empirical findings across Concurrency Scaling (Exp A), Adaptive Overload Control (Exp B), Keep-Alive Efficiency (Exp C), and Static File Sizes (Exp D).
18. **Analysis and Discussion**: Quantitative evaluation of results, latency distribution comparisons ($p50$, $p95$, $p99$), throughput bounds, and admission policy tradeoffs.
19. **Limitations**: Inherent constraints of synchronous worker pools, absence of HTTP/2 multiplexing, single-process limits, and untested network topologies.
20. **Conclusion and Future Enhancements**: Summary of engineering contributions and potential avenues for future research (e.g. io_uring, TLS integration).
21. **References & Appendices**: Complete citations of RFCs, POSIX standards, and supplemental source code listings.
