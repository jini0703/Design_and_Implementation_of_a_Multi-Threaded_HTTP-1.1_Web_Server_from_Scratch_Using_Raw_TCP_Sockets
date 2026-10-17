# Project Presentation Deliverable

## Overview

This directory is the dedicated location for the academic slide deck for the final presentation of the Computer Networks project:

**Project Title**: *Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server from Scratch Using Raw TCP Sockets*

### Expected Deliverable Filename

```text
HTTP11_Web_Server_Project_Presentation.pptx
```

---

## Status & Deliverable Verification

> **Verification Status: COMPLETED & VERIFIED**
> The comprehensive 18-slide academic PowerPoint slide deck has been authored and verified in this directory:
> - **Final Presentation**: [`HTTP11_Web_Server_Project_Presentation.pptx`](HTTP11_Web_Server_Project_Presentation.pptx) (18 slides, 16:9 widescreen, custom dark systems-engineering palette)
> - **Build Generator**: [`generate_presentation.py`](generate_presentation.py)
>
> All 18 slides include structured layout cards, technical bullet points, empirical benchmark comparison charts, live dashboard screenshots, and full oral defense speaker notes. No placeholder slides.

---

## Proposed Presentation Structure

The slide deck is structured into 18 technical slides designed for oral defense before academic faculty and systems-engineering peers:

1. **Title Slide**: Project Title, Course Name, Author Name, Affiliation, and Date.
2. **Motivation & Problem Statement**: The engineering challenge of implementing HTTP/1.1 on raw byte streams without third-party frameworks.
3. **Objectives & Scope**: Core requirements (C17, POSIX sockets, pthreads, RFC 9112) vs. deliberate out-of-scope boundaries.
4. **Computer Networks Fundamentals**: TCP three-way handshake, stream-oriented I/O challenges, and HTTP message boundaries.
5. **TCP Socket Lifecycle**: Socket initialization, `SO_REUSEADDR`, `SO_RCVTIMEO`, `bind`, `listen`, and partial read/write loops.
6. **HTTP/1.1 Request Lifecycle**: Step-by-step tracing from initial TCP accept to worker dispatch, response assembly, and socket reuse/close.
7. **System Architecture**: High-level block diagram showing Listener, Bounded Queue, Worker Pool, Parser, Router, and Metrics engine.
8. **Multithreaded Worker Pool**: Thread lifecycle, eliminating per-request fork cost, and avoiding thread exhaustion.
9. **Bounded Connection Queue**: Circular buffer design, mutex locking, condition variable flow control, and memory bounding.
10. **HTTP Parsing & Framing**: Custom zero-dependency RFC 9112 state machine, header tokenization, and `Content-Length` framing.
11. **Proposed Adaptive Admission Control (QLAAC)**: Three-tier state machine (NORMAL, CAUTION, OVERLOAD), sampling inputs, and hysteresis.
12. **Security & Defensive Engineering**: Path traversal mitigation (`realpath`), buffer overflow defense, timeouts, and descriptor ownership.
13. **Monitoring & Observability**: Real-time `/metrics` telemetry, JSON serialization, and dynamic web dashboard.
14. **Implementation & Live Demonstration**: Tour of the C17 codebase and functional demonstration of static file and API serving.
15. **Benchmarking Methodology**: Experimental setup, tooling (`wrk`), independent variables, and statistical trial execution.
16. **Empirical Results & Analysis**: Presentation of throughput scaling curves, latency CDFs ($p50$, $p95$, $p99$), and QLAAC vs. baseline comparison.
17. **Limitations & Engineering Lessons**: Thread-pool bottlenecks, blocking I/O limits, and insights into systems-level networking.
18. **Conclusion & Future Work**: Project summary, key takeaways, and potential future directions (e.g. event-driven reactor, HTTP/2).
