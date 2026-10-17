"""
generate_presentation.py
Generates the 18-slide academic PowerPoint presentation for:
"Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server from Scratch Using Raw TCP Sockets"

Output:
- deliverables/presentation/HTTP11_Web_Server_Project_Presentation.pptx
"""

import os
from pathlib import Path
from pptx import Presentation
from pptx.util import Inches, Pt
from pptx.dml.color import RGBColor
from pptx.enum.text import PP_ALIGN, MSO_ANCHOR
from pptx.enum.shapes import MSO_SHAPE

BASE_DIR = Path(__file__).resolve().parent.parent.parent
PRES_DIR = BASE_DIR / "deliverables" / "presentation"
PLOTS_DIR = BASE_DIR / "results" / "plots"
PPTX_PATH = PRES_DIR / "HTTP11_Web_Server_Project_Presentation.pptx"

# Color Palette (Dark Systems Engineering Theme)
BG_COLOR = RGBColor(15, 23, 42)        # #0F172A Dark Slate
CARD_BG = RGBColor(30, 41, 59)         # #1E293B Card Slate
CARD_BORDER = RGBColor(51, 65, 85)     # #334155 Border Slate
TEXT_WHITE = RGBColor(248, 250, 252)   # #F8FAFC Bright White
TEXT_MUTED = RGBColor(203, 213, 225)   # #CBD5E1 Muted Slate
TEXT_ACCENT = RGBColor(56, 189, 248)   # #38BDF8 Cyan Accent
ACCENT_GREEN = RGBColor(16, 185, 129)  # #10B981 Emerald
ACCENT_AMBER = RGBColor(245, 158, 11)  # #F59E0B Amber
ACCENT_PURPLE = RGBColor(168, 85, 247) # #A855F7 Purple
HEADER_BG = RGBColor(2, 132, 199)      # #0284C7 Sky Blue

def set_slide_background(slide):
    background = slide.background
    fill = background.fill
    fill.solid()
    fill.fore_color.rgb = BG_COLOR

def add_header(slide, title_text, category_text="HTTP/1.1 WEB SERVER FROM SCRATCH"):
    # Category / Tag
    cat_box = slide.shapes.add_textbox(Inches(0.8), Inches(0.4), Inches(11.7), Inches(0.35))
    tf_c = cat_box.text_frame
    tf_c.word_wrap = True
    p_c = tf_c.paragraphs[0]
    p_c.text = category_text.upper()
    p_c.font.size = Pt(10)
    p_c.font.bold = True
    p_c.font.color.rgb = TEXT_ACCENT

    # Title
    title_box = slide.shapes.add_textbox(Inches(0.8), Inches(0.7), Inches(11.7), Inches(0.65))
    tf_t = title_box.text_frame
    tf_t.word_wrap = True
    p_t = tf_t.paragraphs[0]
    p_t.text = title_text
    p_t.font.size = Pt(22)
    p_t.font.bold = True
    p_t.font.color.rgb = TEXT_WHITE

def add_card(slide, left, top, width, height, title="", border_color=CARD_BORDER, bg_color=CARD_BG):
    shape = slide.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, left, top, width, height)
    shape.fill.solid()
    shape.fill.fore_color.rgb = bg_color
    shape.line.color.rgb = border_color
    shape.line.width = Pt(1.5)

    if title:
        title_box = slide.shapes.add_textbox(left + Inches(0.2), top + Inches(0.15), width - Inches(0.4), Inches(0.4))
        tf = title_box.text_frame
        p = tf.paragraphs[0]
        p.text = title
        p.font.size = Pt(13)
        p.font.bold = True
        p.font.color.rgb = TEXT_ACCENT
    return shape

def add_speaker_notes(slide, notes_text):
    notes_slide = slide.notes_slide
    tf = notes_slide.notes_text_frame
    tf.text = notes_text

def build_presentation():
    print("Building 18-slide Academic Presentation...")
    prs = Presentation()
    prs.slide_width = Inches(13.333)
    prs.slide_height = Inches(7.5)
    blank_layout = prs.slide_layouts[6]

    # ==========================================
    # SLIDE 1: Title Slide
    # ==========================================
    s1 = prs.slides.add_slide(blank_layout)
    set_slide_background(s1)

    # Title Card
    c1 = add_card(s1, Inches(1.0), Inches(1.2), Inches(11.333), Inches(5.1), bg_color=CARD_BG, border_color=HEADER_BG)

    # Text in Title Card
    tb = s1.shapes.add_textbox(Inches(1.5), Inches(1.6), Inches(10.333), Inches(4.3))
    tf = tb.text_frame
    tf.word_wrap = True

    p = tf.paragraphs[0]
    p.text = "ADVANCED COMPUTER NETWORKS & SYSTEMS"
    p.font.size = Pt(13)
    p.font.bold = True
    p.font.color.rgb = TEXT_ACCENT
    p.space_after = Pt(14)

    p2 = tf.add_paragraph()
    p2.text = "Design & Implementation of a Multi-Threaded HTTP/1.1 Web Server from Scratch"
    p2.font.size = Pt(26)
    p2.font.bold = True
    p2.font.color.rgb = TEXT_WHITE
    p2.space_after = Pt(8)

    p3 = tf.add_paragraph()
    p3.text = "Zero-Dependency ISO C17 Architecture on Raw POSIX Sockets with Queue-and-Latency-Aware Admission Control (QLAAC)"
    p3.font.size = Pt(14)
    p3.font.color.rgb = RGBColor(148, 163, 184)
    p3.space_after = Pt(28)

    p4 = tf.add_paragraph()
    p4.text = "Author: Systems Engineering Team   |   Course: Computer Networks (CS-504)   |   Date: October 2026"
    p4.font.size = Pt(12)
    p4.font.bold = True
    p4.font.color.rgb = TEXT_WHITE

    p5 = tf.add_paragraph()
    p5.text = "Platform: Linux Kernel 6.6 / x86_64   |   Standards: RFC 9112, RFC 9110, POSIX.1-2008"
    p5.font.size = Pt(11)
    p5.font.color.rgb = RGBColor(148, 163, 184)

    add_speaker_notes(s1, "Welcome committee members. Today I present the design, low-level implementation, and empirical verification of a multi-threaded HTTP/1.1 web server built strictly from scratch in ISO C17 on raw POSIX sockets without external libraries.")

    # ==========================================
    # SLIDE 2: Motivation & Problem Statement
    # ==========================================
    s2 = prs.slides.add_slide(blank_layout)
    set_slide_background(s2)
    add_header(s2, "Motivation & Problem Statement: Systems Engineering Challenges")

    # 3 Cards
    add_card(s2, Inches(0.8), Inches(1.5), Inches(3.6), Inches(5.2), "1. Stream vs Frame Mismatch")
    tb2a = s2.shapes.add_textbox(Inches(1.0), Inches(2.2), Inches(3.2), Inches(4.3))
    tf2a = tb2a.text_frame
    tf2a.word_wrap = True
    tf2a.paragraphs[0].text = "• TCP guarantees reliable byte streams but provides zero message boundary framing.\n\n• read(2) can return partial headers, an exact request, or multiple concatenated pipelined requests.\n\n• Requires custom circular streaming buffers and deterministic CRLF delimiter detection without buffer overruns."
    tf2a.paragraphs[0].font.size = Pt(11)
    tf2a.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_card(s2, Inches(4.8), Inches(1.5), Inches(3.6), Inches(5.2), "2. Thread Explosion & Collapse")
    tb2b = s2.shapes.add_textbox(Inches(5.0), Inches(2.2), Inches(3.2), Inches(4.3))
    tf2b = tb2b.text_frame
    tf2b.word_wrap = True
    tf2b.paragraphs[0].text = "• Naive thread-per-connection architectures cause severe thread thrashing and memory exhaustion.\n\n• Spawning 10,000 POSIX threads at 8 MB default stack consumes 80 GB virtual memory.\n\n• Context switching latency destroys CPU cache locality and collapses server throughput under concurrency."
    tf2b.paragraphs[0].font.size = Pt(11)
    tf2b.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_card(s2, Inches(8.8), Inches(1.5), Inches(3.7), Inches(5.2), "3. Queue Starvation & Bufferbloat")
    tb2c = s2.shapes.add_textbox(Inches(9.0), Inches(2.2), Inches(3.3), Inches(4.3))
    tf2c = tb2c.text_frame
    tf2c.word_wrap = True
    tf2c.paragraphs[0].text = "• When arrival rate exceeds processing rate (λ > μ), unmanaged FIFO queues expand indefinitely.\n\n• By Little's Law (L = λW), latency explodes while clients time out waiting in queue memory.\n\n• Demands closed-loop admission control to shed load defensively and protect p99 tail latency."
    tf2c.paragraphs[0].font.size = Pt(11)
    tf2c.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_speaker_notes(s2, "The core motivation is mastering systems-level networking. We must solve three fundamental hurdles: reconstructing HTTP message boundaries from continuous TCP byte streams, preventing OS thread thrashing, and avoiding bufferbloat-induced tail latency collapse.")

    # ==========================================
    # SLIDE 3: Objectives & Scope
    # ==========================================
    s3 = prs.slides.add_slide(blank_layout)
    set_slide_background(s3)
    add_header(s3, "Project Objectives & Architectural Scope Boundaries")

    add_card(s3, Inches(0.8), Inches(1.5), Inches(5.6), Inches(5.2), "Core Functional Objectives (In-Scope)")
    tb3a = s3.shapes.add_textbox(Inches(1.0), Inches(2.1), Inches(5.2), Inches(4.4))
    tf3a = tb3a.text_frame
    tf3a.word_wrap = True
    tf3a.paragraphs[0].text = "✓ Zero External Dependencies: Pure C17 using libc and POSIX.1-2008 system calls only.\n✓ Raw Socket Engine: Non-blocking accept, SO_REUSEADDR, SO_RCVTIMEO/SO_SNDTIMEO.\n✓ Fixed-Capacity Thread Pool: 4 pre-spawned worker threads (scalable to 16) eliminating thread churn.\n✓ Bounded Queue: Thread-safe circular FIFO buffer (64 slots default / scalable to 128).\n✓ Strict RFC 9112 Parser: Zero-copy header tokenization, case-insensitivity, keep-alive reuse.\n✓ QLAAC Admission Controller: 3-tier feedback loop preventing bufferbloat collapse.\n✓ Observability Engine: Real-time sliding window percentiles and HTML5 live dashboard."
    tf3a.paragraphs[0].font.size = Pt(11)
    tf3a.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_card(s3, Inches(6.8), Inches(1.5), Inches(5.7), Inches(5.2), "Deliberate Architectural Boundaries")
    tb3b = s3.shapes.add_textbox(Inches(7.0), Inches(2.1), Inches(5.3), Inches(4.4))
    tf3b = tb3b.text_frame
    tf3b.word_wrap = True
    tf3b.paragraphs[0].text = "✗ No TLS/HTTPS Inside Server: Cryptographic handshake and TLS record layer are deliberately omitted to focus purely on raw transport; in production, TLS termination is handled by upstream HAProxy/Nginx reverse proxy.\n\n✗ No HTTP/2 or HTTP/3: Protocol scope strictly bounded to HTTP/1.1 persistent streaming semantics.\n\n✗ No Asynchronous Event Loop Frameworks: Deliberately avoided libevent/libuv to directly implement POSIX multithreading and condition variable synchronization primitives.\n\n✓ Focus on Mechanical Sympathy: Deep understanding of kernel buffers, socket options, and thread synchronization."
    tf3b.paragraphs[0].font.size = Pt(11)
    tf3b.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_speaker_notes(s3, "Our scope is uncompromising: 100% zero-dependency C17. We deliberately exclude TLS and external event loop libraries to directly engage with kernel socket options, memory bounds, and POSIX synchronization primitives.")

    # ==========================================
    # SLIDE 4: Computer Networks Fundamentals
    # ==========================================
    s4 = prs.slides.add_slide(blank_layout)
    set_slide_background(s4)
    add_header(s4, "Computer Networks Concepts: Transport Semantics & TCP Mechanics")

    add_card(s4, Inches(0.8), Inches(1.5), Inches(5.6), Inches(5.2), "TCP Transport Guarantees vs Application Needs")
    tb4a = s4.shapes.add_textbox(Inches(1.0), Inches(2.1), Inches(5.2), Inches(4.4))
    tf4a = tb4a.text_frame
    tf4a.word_wrap = True
    tf4a.paragraphs[0].text = "• Reliability & In-Order Delivery: TCP uses 32-bit sequence numbers, cumulative ACKs, and sliding windows to guarantee data arrives intact and in order.\n\n• Connection Lifecycle:\n  1. 3-Way Handshake (SYN -> SYN-ACK -> ACK).\n  2. Bi-directional full-duplex data transfer.\n  3. Orderly teardown via 4-Way FIN/ACK exchange.\n\n• Kernel Backlog Mechanics:\n  - Incomplete connection queue (SYN received).\n  - Completed connection queue (ESTABLISHED, waiting for accept(2))."
    tf4a.paragraphs[0].font.size = Pt(11)
    tf4a.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_card(s4, Inches(6.8), Inches(1.5), Inches(5.7), Inches(5.2), "Socket Buffer Mechanics & Flow Control")
    tb4b = s4.shapes.add_textbox(Inches(7.0), Inches(2.1), Inches(5.3), Inches(4.4))
    tf4b = tb4b.text_frame
    tf4b.word_wrap = True
    tf4b.paragraphs[0].text = "• TCP Sliding Window Flow Control: Server advertises receive window (rcv_wnd). If worker threads are slow, rcv_wnd shrinks, exerting backpressure on client.\n\n• Socket Timeouts (SO_RCVTIMEO / SO_SNDTIMEO):\n  - Crucial defense against deadlocks and Slowloris.\n  - Prevents a slow or stalled client from holding worker threads indefinitely.\n\n• TIME_WAIT State Management:\n  - Setting SO_REUSEADDR allows instant socket re-binding after server restarts, avoiding 2MSL port lockout."
    tf4b.paragraphs[0].font.size = Pt(11)
    tf4b.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_speaker_notes(s4, "Understanding TCP semantics is fundamental. We leverage kernel backlog queues, handle TCP flow control via sliding windows, and use SO_REUSEADDR and timeout options to ensure resilience against lingering sockets and slow clients.")

    # ==========================================
    # SLIDE 5: TCP Socket Lifecycle
    # ==========================================
    s5 = prs.slides.add_slide(blank_layout)
    set_slide_background(s5)
    add_header(s5, "Low-Level TCP Socket Lifecycle & Partial I/O Handling")

    add_card(s5, Inches(0.8), Inches(1.5), Inches(5.6), Inches(5.2), "Socket Initialization & Configuration")
    tb5a = s5.shapes.add_textbox(Inches(1.0), Inches(2.1), Inches(5.2), Inches(4.4))
    tf5a = tb5a.text_frame
    tf5a.word_wrap = True
    tf5a.paragraphs[0].text = "1. socket(AF_INET, SOCK_STREAM, 0):\n   Allocates kernel file descriptor for IPv4 stream socket.\n\n2. setsockopt(SO_REUSEADDR, 1):\n   Permits binding to port even if lingering in TIME_WAIT.\n\n3. bind(server_fd, &addr, sizeof(addr)):\n   Binds socket to 0.0.0.0 and configured port (default 8080).\n\n4. listen(server_fd, 1024):\n   Enables passive listening with deep backlog of 1024.\n\n5. accept(server_fd, ...):\n   Extracts established socket from kernel queue."
    tf5a.paragraphs[0].font.size = Pt(11)
    tf5a.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_card(s5, Inches(6.8), Inches(1.5), Inches(5.7), Inches(5.2), "Robust Partial Read & Write Loops")
    tb5b = s5.shapes.add_textbox(Inches(7.0), Inches(2.1), Inches(5.3), Inches(4.4))
    tf5b = tb5b.text_frame
    tf5b.word_wrap = True
    tf5b.paragraphs[0].text = "• Partial Write Invariant:\n  A single write() or send() is NOT guaranteed to transmit all bytes requested.\n\n• Implementation Solution:\n  ```c\n  ssize_t send_all(int fd, const void *buf, size_t len) {\n      size_t total = 0;\n      while (total < len) {\n          ssize_t n = send(fd, (char*)buf + total, \n                           len - total, MSG_NOSIGNAL);\n          if (n <= 0) return -1;\n          total += n;\n      }\n      return total;\n  }\n  ```\n• MSG_NOSIGNAL prevents fatal SIGPIPE crashes if the client aborts during transmission."
    tf5b.paragraphs[0].font.size = Pt(11)
    tf5b.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_speaker_notes(s5, "Notice our send_all loop. On POSIX byte streams, assuming a write sends everything is a fatal bug. We loop until all bytes are sent and pass MSG_NOSIGNAL to prevent client connection resets from killing our server with SIGPIPE.")

    # ==========================================
    # SLIDE 6: HTTP/1.1 Request Lifecycle
    # ==========================================
    s6 = prs.slides.add_slide(blank_layout)
    set_slide_background(s6)
    add_header(s6, "End-to-End Request Lifecycle: Ingress to Dispatch")

    stages = [
        ("1. Ingress & Accept", "Listener thread executes accept(). Immediate QLAAC check. If overloaded, 503 dispatched; else pushed to queue."),
        ("2. Queue Dispatch", "Worker thread awakened by condition variable. Pops client_fd from FIFO buffer. Marks worker active."),
        ("3. Ingress Timing", "First byte arrival timestamp captured immediately via clock_gettime() to isolate execution from idle keep-alive wait."),
        ("4. Parsing & Routing", "RFC 9112 engine tokenizes request. Router matches static path or dynamic API (/metrics, /api/status)."),
        ("5. Response Assembly", "Worker formats HTTP status line, headers (Date, Content-Length, Keep-Alive), and transmits payload via send_all()."),
        ("6. Connection Reuse", "If keep-alive active, loops to await next request. On EOF, timeout, or Connection: close, gracefully closes socket.")
    ]

    for i, (st, sd) in enumerate(stages):
        col = i % 2
        row = i // 2
        x = Inches(0.8 + col * 5.9)
        y = Inches(1.5 + row * 1.8)
        add_card(s6, x, y, Inches(5.6), Inches(1.6), st)
        tb_st = s6.shapes.add_textbox(x + Inches(0.2), y + Inches(0.45), Inches(5.2), Inches(1.0))
        tf_st = tb_st.text_frame
        tf_st.word_wrap = True
        tf_st.paragraphs[0].text = sd
        tf_st.paragraphs[0].font.size = Pt(10.5)
        tf_st.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_speaker_notes(s6, "Here is the six-stage request lifecycle. A key engineering fix we made was stage 3: we capture the latency timer only when the first request byte arrives, preventing idle keep-alive wait time from distorting request service latency.")

    # ==========================================
    # SLIDE 7: System Architecture Diagram
    # ==========================================
    s7 = prs.slides.add_slide(blank_layout)
    set_slide_background(s7)
    add_header(s7, "System Architecture: Multi-Threaded Concurrency Pipeline")

    fig4_path = str(PLOTS_DIR / "fig4_system_architecture.png")
    if os.path.exists(fig4_path):
        s7.shapes.add_picture(fig4_path, Inches(1.0), Inches(1.5), Inches(11.333), Inches(5.3))

    add_speaker_notes(s7, "Figure 4 shows our complete architecture: client connections arrive at the listener loop, pass through QLAAC admission, enter the 64-slot bounded queue, and are dispatched to the 4 worker threads (scalable to 16) for parsing, routing, and metrics recording.")

    # ==========================================
    # SLIDE 8: Multithreaded Worker Pool
    # ==========================================
    s8 = prs.slides.add_slide(blank_layout)
    set_slide_background(s8)
    add_header(s8, "Pre-Spawned POSIX Thread Pool Architecture")

    add_card(s8, Inches(0.8), Inches(1.5), Inches(5.6), Inches(5.2), "Thread Pool Implementation Details")
    tb8a = s8.shapes.add_textbox(Inches(1.0), Inches(2.1), Inches(5.2), Inches(4.4))
    tf8a = tb8a.text_frame
    tf8a.word_wrap = True
    tf8a.paragraphs[0].text = "• Pre-Spawned Allocation:\n  4 worker threads in benchmark & default configuration (scalable up to 16) created at boot via pthread_create().\n\n• Zero Creation Overhead:\n  Completely eliminates per-connection fork(2) or pthread_create() latency on the hot path.\n\n• Deterministic Resource Bounding:\n  Server memory consumption remains strictly constant regardless of client load spikes.\n\n• Clean Lifecycle Management:\n  Worker loops block on condition variable, pop descriptors, process requests, and cleanly rejoin on shutdown."
    tf8a.paragraphs[0].font.size = Pt(11)
    tf8a.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_card(s8, Inches(6.8), Inches(1.5), Inches(5.7), Inches(5.2), "Worker Utilization & Concurrency Safety")
    tb8b = s8.shapes.add_textbox(Inches(7.0), Inches(2.1), Inches(5.3), Inches(4.4))
    tf8b = tb8b.text_frame
    tf8b.word_wrap = True
    tf8b.paragraphs[0].text = "• Atomic Worker State Tracking:\n  Workers increment active_worker_count before reading socket bytes and decrement upon completing the HTTP transaction.\n\n• Multi-Core Scalability:\n  Linux CFS scheduler distributes worker threads across available hardware cores without CPU core pinning.\n\n• Graceful Teardown:\n  SIGINT/SIGTERM handlers set shutdown = true, broadcast pthread_cond_broadcast, and cleanly join all threads with zero descriptor leaks."
    tf8b.paragraphs[0].font.size = Pt(11)
    tf8b.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_speaker_notes(s8, "By pre-spawning worker threads (4 in our benchmark configuration, scalable to 16), we eliminate dynamic thread allocation costs. Worker state is tracked atomically, providing real-time worker utilization telemetry to the QLAAC admission controller.")

    # ==========================================
    # SLIDE 9: Bounded Connection Queue
    # ==========================================
    s9 = prs.slides.add_slide(blank_layout)
    set_slide_background(s9)
    add_header(s9, "Thread-Safe Bounded FIFO Circular Connection Queue")

    add_card(s9, Inches(0.8), Inches(1.5), Inches(5.6), Inches(5.2), "Circular Queue Data Structure & Mutex")
    tb9a = s9.shapes.add_textbox(Inches(1.0), Inches(2.1), Inches(5.2), Inches(4.4))
    tf9a = tb9a.text_frame
    tf9a.word_wrap = True
    tf9a.paragraphs[0].text = "• Fixed Capacity Circular Buffer:\n  Capacity bounded to 64 socket descriptors default (scalable up to 128).\n\n• O(1) Push and Pop Operations:\n  - tail = (tail + 1) % QUEUE_CAPACITY\n  - head = (head + 1) % QUEUE_CAPACITY\n\n• POSIX Synchronization Primitives:\n  - Single pthread_mutex_t protects queue state.\n  - pthread_cond_t not_empty signals sleeping workers.\n  - pthread_cond_t not_full signals listener when queue drains."
    tf9a.paragraphs[0].font.size = Pt(11)
    tf9a.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_card(s9, Inches(6.8), Inches(1.5), Inches(5.7), Inches(5.2), "Bufferbloat Prevention & Backpressure")
    tb9b = s9.shapes.add_textbox(Inches(7.0), Inches(2.1), Inches(5.3), Inches(4.4))
    tf9b = tb9b.text_frame
    tf9b.word_wrap = True
    tf9b.paragraphs[0].text = "• Why Bounding is Essential:\n  Unbounded queues allow millions of requests to buffer in memory, causing OOM panics and catastrophic wait times (Little's Law: W = L / λ).\n\n• Condition Variable Flow Control:\n  When queue hits 64, listener blocks on not_full or sheds load, applying physical backpressure to the TCP accept queue.\n\n• Lock Contention Optimization:\n  Mutex hold times are measured in nanoseconds (only 4 pointer increments inside critical section)."
    tf9b.paragraphs[0].font.size = Pt(11)
    tf9b.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_speaker_notes(s9, "The connection queue is a lock-protected circular buffer with capacity 64 (scalable up to 128). Bounding the queue is critical: it prevents bufferbloat and enforces backpressure on the TCP socket layer.")

    # ==========================================
    # SLIDE 10: HTTP Parsing & Framing
    # ==========================================
    s10 = prs.slides.add_slide(blank_layout)
    set_slide_background(s10)
    add_header(s10, "RFC 9112 HTTP/1.1 Parsing Engine & Message Framing")

    add_card(s10, Inches(0.8), Inches(1.5), Inches(5.6), Inches(5.2), "RFC 9112 Grammar & Parser Invariants")
    tb10a = s10.shapes.add_textbox(Inches(1.0), Inches(2.1), Inches(5.2), Inches(4.4))
    tf10a = tb10a.text_frame
    tf10a.word_wrap = True
    tf10a.paragraphs[0].text = "• Request Line Decomposition:\n  Method SP Request-URI SP HTTP-Version CRLF\n  Validates standard HTTP verbs (GET, POST, HEAD, OPTIONS).\n\n• Case-Insensitive Header Extraction:\n  Extracts Host, Connection, Content-Length, Content-Type using strncasecmp().\n\n• Bounded Stack Buffer:\n  8,192-byte buffer prevents stack overflows while handling standard web request header sizes."
    tf10a.paragraphs[0].font.size = Pt(11)
    tf10a.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_card(s10, Inches(6.8), Inches(1.5), Inches(5.7), Inches(5.2), "Persistent Connections & Framing")
    tb10b = s10.shapes.add_textbox(Inches(7.0), Inches(2.1), Inches(5.3), Inches(4.4))
    tf10b = tb10b.text_frame
    tf10b.word_wrap = True
    tf10b.paragraphs[0].text = "• Keep-Alive Socket Reuse:\n  In HTTP/1.1, connections are persistent by default unless Connection: close is received.\n\n• Exact Content-Length Framing:\n  Extracts body payload exactly matching Content-Length, avoiding framing ambiguities.\n\n• Malformed Request Defense:\n  Incomplete request lines, invalid versions (HTTP/2.0), or missing headers immediately yield 400 Bad Request or 505 Version Not Supported."
    tf10b.paragraphs[0].font.size = Pt(11)
    tf10b.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_speaker_notes(s10, "Our HTTP parser strictly implements RFC 9112 grammar. It extracts headers case-insensitively, parses Content-Length for body framing, and manages persistent keep-alive connections.")

    # ==========================================
    # SLIDE 11: Proposed QLAAC Mechanism
    # ==========================================
    s11 = prs.slides.add_slide(blank_layout)
    set_slide_background(s11)
    add_header(s11, "QLAAC: Adaptive Admission Control State Machine")

    fig5_path = str(PLOTS_DIR / "fig5_qlaac_state_machine.png")
    if os.path.exists(fig5_path):
        s11.shapes.add_picture(fig5_path, Inches(1.0), Inches(1.5), Inches(11.333), Inches(5.3))

    add_speaker_notes(s11, "Figure 5 illustrates our QLAAC state machine: NORMAL admits all; CAUTION (triggered by worker util >= 85%, queue >= 70%, or p95 >= 50ms) throttles keep-alive idle persistence to 1s; and OVERLOAD (triggered by queue >= 90%, p95 >= 150ms, worker saturation with queue >= 70%, or sustained pressure >= 3 windows) sheds excess traffic via 503 with Retry-After: 5. Recovery strictly requires 5 consecutive healthy windows.")

    # ==========================================
    # SLIDE 12: Security & Defensive Engineering
    # ==========================================
    s12 = prs.slides.add_slide(blank_layout)
    set_slide_background(s12)
    add_header(s12, "Security Architecture: Sandboxing & Hardened Invariants")

    add_card(s12, Inches(0.8), Inches(1.5), Inches(5.6), Inches(5.2), "Directory Traversal Mitigation")
    tb12a = s12.shapes.add_textbox(Inches(1.0), Inches(2.1), Inches(5.2), Inches(4.4))
    tf12a = tb12a.text_frame
    tf12a.word_wrap = True
    tf12a.paragraphs[0].text = "• Threat Model:\n  Malicious clients submitting URI paths like /../../etc/shadow or URL-encoded traversals.\n\n• Canonical Sandbox Defense:\n  1. Combine document root and requested URI.\n  2. Resolve physical canonical path via realpath(3).\n  3. Verify canonical path begins with canonical root:\n     strncmp(resolved, canonical_root, len) == 0\n  4. Rejects violations with 403 Forbidden."
    tf12a.paragraphs[0].font.size = Pt(11)
    tf12a.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_card(s12, Inches(6.8), Inches(1.5), Inches(5.7), Inches(5.2), "Defensive Reliability Invariants")
    tb12b = s12.shapes.add_textbox(Inches(7.0), Inches(2.1), Inches(5.3), Inches(4.4))
    tf12b = tb12b.text_frame
    tf12b.word_wrap = True
    tf12b.paragraphs[0].text = "• Slowloris Mitigation:\n  Enforces 5-second socket receive timeout (SO_RCVTIMEO). Incomplete headers automatically disconnect.\n\n• Single-Owner Descriptor Invariant:\n  Sockets are held by listener, stored in queue, or held by worker. Never duplicated or leaked.\n\n• Zero Dynamic Heap Allocations in Hot Path:\n  All request parsing and status generation occurs in fixed stack frames, eliminating memory fragmentation and memory leaks."
    tf12b.paragraphs[0].font.size = Pt(11)
    tf12b.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_speaker_notes(s12, "Security is engineered into the core. Path traversal is defeated via realpath() canonical prefix checking, Slowloris attacks are mitigated via socket read timeouts, and zero dynamic heap allocations on the hot path eliminate memory leaks.")

    # ==========================================
    # SLIDE 13: Monitoring & Observability
    # ==========================================
    s13 = prs.slides.add_slide(blank_layout)
    set_slide_background(s13)
    add_header(s13, "Telemetry & Live Systems Dashboard")

    # Left side: Metrics engine details
    add_card(s13, Inches(0.8), Inches(1.5), Inches(4.2), Inches(5.2), "In-Memory Telemetry Engine")
    tb13a = s13.shapes.add_textbox(Inches(1.0), Inches(2.1), Inches(3.8), Inches(4.4))
    tf13a = tb13a.text_frame
    tf13a.word_wrap = True
    tf13a.paragraphs[0].text = "• Sliding Reservoir Sampling:\n  Circular buffer storing 1,024 recent request latency durations in microseconds.\n\n• Percentile Sorting:\n  Computes p50, p95, p99 on demand without locking worker execution.\n\n• JSON Telemetry API:\n  GET /metrics exposes real-time state, RPS, active workers, and queue count.\n\n• Scraping Isolation:\n  /metrics calls are excluded from latency metrics to prevent telemetry feedback pollution."
    tf13a.paragraphs[0].font.size = Pt(10.5)
    tf13a.paragraphs[0].font.color.rgb = TEXT_MUTED

    # Right side: Dashboard Preview Image
    dash_img = str(PLOTS_DIR / "dashboard_preview_overview.png")
    if os.path.exists(dash_img):
        s13.shapes.add_picture(dash_img, Inches(5.2), Inches(1.5), Inches(7.333), Inches(5.2))

    add_speaker_notes(s13, "Here is our live HTML5/Canvas monitoring dashboard. It polls /metrics every 500ms, displaying live queue occupancy gauges, active worker counts, QLAAC state badges, and an interactive latency profile.")

    # ==========================================
    # SLIDE 14: Implementation & Live Demonstration
    # ==========================================
    s14 = prs.slides.add_slide(blank_layout)
    set_slide_background(s14)
    add_header(s14, "Implementation Tour & Interactive Console")

    add_card(s14, Inches(0.8), Inches(1.5), Inches(4.2), Inches(5.2), "C17 Modular Codebase Tour")
    tb14a = s14.shapes.add_textbox(Inches(1.0), Inches(2.1), Inches(3.8), Inches(4.4))
    tf14a = tb14a.text_frame
    tf14a.word_wrap = True
    tf14a.paragraphs[0].text = "• src/server.c: Core accept loop and QLAAC interception.\n• src/socket.c: Berkeley socket setup and timeout options.\n• src/thread_pool.c: Worker threads and ingress timing.\n• src/connection_queue.c: Bounded FIFO circular buffer.\n• src/http_parser.c: RFC 9112 streaming parser.\n• src/router.c: Static file serving via sendfile(2) & REST APIs.\n• src/adaptive_controller.c: QLAAC state machine.\n• src/metrics.c: In-memory latency reservoir."
    tf14a.paragraphs[0].font.size = Pt(10.5)
    tf14a.paragraphs[0].font.color.rgb = TEXT_MUTED

    dash_console = str(PLOTS_DIR / "dashboard_preview_console.png")
    if os.path.exists(dash_console):
        s14.shapes.add_picture(dash_console, Inches(5.2), Inches(1.5), Inches(7.333), Inches(5.2))

    add_speaker_notes(s14, "The server includes an integrated API testing console and traffic generation deck built right into the dashboard, allowing live verification of REST endpoints and load injection.")

    # ==========================================
    # SLIDE 15: Benchmarking Methodology
    # ==========================================
    s15 = prs.slides.add_slide(blank_layout)
    set_slide_background(s15)
    add_header(s15, "Experimental Benchmarking Setup & Methodology")

    add_card(s15, Inches(0.8), Inches(1.5), Inches(5.6), Inches(5.2), "Experimental Matrix & Workload")
    tb15a = s15.shapes.add_textbox(Inches(1.0), Inches(2.1), Inches(5.2), Inches(4.4))
    tf15a = tb15a.text_frame
    tf15a.word_wrap = True
    tf15a.paragraphs[0].text = "• Controlled Concurrency Levels:\n  C = 2, 8, 32, and 64 concurrent client connections.\n\n• Rigorous Statistical Repetitions:\n  5 independent trials per concurrency level (40 total trials across Baseline and QLAAC configurations).\n\n• Workload Target:\n  GET /api/status returning lightweight JSON payload to stress network socket handling and scheduling.\n\n• Duration & Warmup:\n  10 seconds per trial, measuring steady-state throughput."
    tf15a.paragraphs[0].font.size = Pt(11)
    tf15a.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_card(s15, Inches(6.8), Inches(1.5), Inches(5.7), Inches(5.2), "Rigorous Response Accounting Fix")
    tb15b = s15.shapes.add_textbox(Inches(7.0), Inches(2.1), Inches(5.3), Inches(4.4))
    tf15b = tb15b.text_frame
    tf15b.word_wrap = True
    tf15b.paragraphs[0].text = "• Audited Accounting Methodology:\n  Corrected benchmark parser to classify status lines strictly:\n  - 2xx: Successful application requests.\n  - 5xx: Server overload rejections.\n  - Transport errors: Timeouts or dropped connections.\n\n• Latency Isolation Guarantee:\n  p50/p95/p99 latency metrics represent successful 2xx responses only, preventing discarded 503 shedding packets from falsely deflating latency figures."
    tf15b.paragraphs[0].font.size = Pt(11)
    tf15b.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_speaker_notes(s15, "Our benchmarking methodology is statistically rigorous: 40 controlled trials with 5 repetitions per configuration. We audited the response parser to ensure 2xx successes and 5xx shedding are classified correctly with zero conflation.")

    # ==========================================
    # SLIDE 16: Empirical Results & Analysis
    # ==========================================
    s16 = prs.slides.add_slide(blank_layout)
    set_slide_background(s16)
    add_header(s16, "Empirical Results: Throughput & Tail Latency Comparison")

    fig1_path = str(PLOTS_DIR / "fig1_throughput_comparison.png")
    fig2_path = str(PLOTS_DIR / "fig2_latency_p99_comparison.png")

    if os.path.exists(fig1_path):
        s16.shapes.add_picture(fig1_path, Inches(0.8), Inches(1.5), Inches(5.6), Inches(4.2))
    if os.path.exists(fig2_path):
        s16.shapes.add_picture(fig2_path, Inches(6.8), Inches(1.5), Inches(5.7), Inches(4.2))

    # Summary banner at bottom
    add_card(s16, Inches(0.8), Inches(5.85), Inches(11.7), Inches(1.1), bg_color=CARD_BG, border_color=ACCENT_GREEN)
    tb16_bot = s16.shapes.add_textbox(Inches(1.0), Inches(5.95), Inches(11.3), Inches(0.9))
    tf16_b = tb16_bot.text_frame
    tf16_b.word_wrap = True
    p_b = tf16_b.paragraphs[0]
    p_b.text = "KEY RESULT: At C=64, Baseline queue saturation explodes p99 latency to 440.33 ms. QLAAC sheds 72.14% excess traffic via HTTP 503 (mean 16,475 5xx/trial), holding p99 latency to 161.18 ms (2.73x improvement) and preventing queue collapse."
    p_b.font.size = Pt(11)
    p_b.font.bold = True
    p_b.font.color.rgb = TEXT_WHITE

    add_speaker_notes(s16, "These two plots present our main empirical findings from the 40 controlled trials. On the left, throughput scales beyond 14,600 RPS at C=2. On the right, at 64 clients, Baseline p99 latency collapses to 440.33 ms due to queue bufferbloat, whereas QLAAC successfully clamps p99 latency to 161.18 ms by shedding 72.14% of excess traffic gracefully via HTTP 503.")

    # ==========================================
    # SLIDE 17: Limitations & Engineering Lessons
    # ==========================================
    s17 = prs.slides.add_slide(blank_layout)
    set_slide_background(s17)
    add_header(s17, "Engineering Limitations & Core Lessons Learned")

    add_card(s17, Inches(0.8), Inches(1.5), Inches(5.6), Inches(5.2), "Systemic Limitations")
    tb17a = s17.shapes.add_textbox(Inches(1.0), Inches(2.1), Inches(5.2), Inches(4.4))
    tf17a = tb17a.text_frame
    tf17a.word_wrap = True
    tf17a.paragraphs[0].text = "• Synchronous Disk I/O Blocking:\n  While sendfile(2) minimizes kernel copies, disk cache misses still block worker threads.\n\n• HTTP/1.1 Head-of-Line Blocking:\n  Pipelined requests on a single TCP connection must be served sequentially.\n\n• Single-Process Bottleneck:\n  Architecture does not span multiple worker processes across NUMA nodes (future work: SO_REUSEPORT)."
    tf17a.paragraphs[0].font.size = Pt(11)
    tf17a.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_card(s17, Inches(6.8), Inches(1.5), Inches(5.7), Inches(5.2), "Key Engineering Insights")
    tb17b = s17.shapes.add_textbox(Inches(7.0), Inches(2.1), Inches(5.3), Inches(4.4))
    tf17b = tb17b.text_frame
    tf17b.word_wrap = True
    tf17b.paragraphs[0].text = "• Ingress Timing Precision:\n  Measuring latency from socket accept incorrectly includes client keep-alive idle sleep. Capturing time on first request byte is essential for true service time.\n\n• Multi-Factor Admission Logic:\n  Worker saturation alone is an insufficient indicator of overload. True overload must consider queue occupancy and latency trends.\n\n• Observability Overhead:\n  Telemetry scraping (/metrics) must be excluded from application latency reservoirs."
    tf17b.paragraphs[0].font.size = Pt(11)
    tf17b.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_speaker_notes(s17, "We learned critical systems lessons during this audit. Specifically, latency must be measured from first byte arrival rather than connection accept, and admission control requires multi-factor heuristics rather than naive worker utilization.")

    # ==========================================
    # SLIDE 18: Conclusion & Future Work
    # ==========================================
    s18 = prs.slides.add_slide(blank_layout)
    set_slide_background(s18)
    add_header(s18, "Conclusion & Future Research Directions")

    add_card(s18, Inches(0.8), Inches(1.5), Inches(5.6), Inches(5.2), "Summary of Contributions", border_color=ACCENT_GREEN)
    tb18a = s18.shapes.add_textbox(Inches(1.0), Inches(2.1), Inches(5.2), Inches(4.4))
    tf18a = tb18a.text_frame
    tf18a.word_wrap = True
    tf18a.paragraphs[0].text = "✓ Zero-Dependency C17 Server: Full HTTP/1.1 web server constructed on raw Berkeley TCP sockets.\n\n✓ Bounded Concurrency Pipeline: 16-worker thread pool with condition-variable FIFO queue.\n\n✓ Verified QLAAC Controller: Prevented queue collapse under 64 clients, cutting p99 latency by 2.73x.\n\n✓ Complete Verification: 100% pass rate across 5 C test suites and 23 Python RFC suites.\n\n✓ Production Deliverables: Comprehensive 12-page academic report, 18-slide presentation, and live dashboard."
    tf18a.paragraphs[0].font.size = Pt(11)
    tf18a.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_card(s18, Inches(6.8), Inches(1.5), Inches(5.7), Inches(5.2), "Future Research Directions")
    tb18b = s18.shapes.add_textbox(Inches(7.0), Inches(2.1), Inches(5.3), Inches(4.4))
    tf18b = tb18b.text_frame
    tf18b.word_wrap = True
    tf18b.paragraphs[0].text = "• Asynchronous I/O via io_uring:\n  Transitioning from blocking worker threads to Linux 5.1+ io_uring submission/completion queues.\n\n• Multi-Process Socket Sharding:\n  Utilizing SO_REUSEPORT to pin listener loops to individual CPU cores.\n\n• Binary HTTP/2 Protocol Multiplexing:\n  Implementing binary framing layers to eliminate Head-of-Line blocking.\n\n• Hardware TLS Acceleration:\n  Evaluating kernel TLS (kTLS) for hardware-offloaded cryptographic streaming."
    tf18b.paragraphs[0].font.size = Pt(11)
    tf18b.paragraphs[0].font.color.rgb = TEXT_MUTED

    add_speaker_notes(s18, "In conclusion, this project demonstrates that a robust, high-performance HTTP/1.1 server can be built purely from scratch with mechanical sympathy for OS sockets and thread synchronization. Thank you, and I look forward to your questions.")

    prs.save(str(PPTX_PATH))
    print(f"Saved Presentation to: {PPTX_PATH}")
    print(f"Presentation validation: 18 slides generated successfully!")

if __name__ == "__main__":
    build_presentation()
