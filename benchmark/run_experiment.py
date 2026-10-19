#!/usr/bin/env python3
"""
run_experiment.py
Benchmarking orchestrator and experiment harness for the HTTP/1.1 Web Server.

Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
         from Scratch Using Raw TCP Sockets

Metrics Accounting Specification:
 - attempted_requests: Total request transmissions initiated by benchmark workers.
 - completed_http_responses: Fully received and framed HTTP responses (2xx + 3xx + 4xx + 5xx).
 - success_2xx: Successfully received HTTP 2xx status responses.
 - http_3xx: HTTP 3xx redirection responses.
 - http_4xx: HTTP 4xx client error responses.
 - http_5xx: HTTP 5xx server error responses (including 503 overload shedding).
 - transport_errors: Connection refused, reset by peer, or network pipe aborts.
 - timeouts: Socket connect or response read timeouts.
 - invalid_responses: Truncated frames, malformed status lines, or syntax corruptions.
 - successful_rps: Measured throughput of verified 2xx responses (success_2xx / duration_sec).
 - latency statistics (avg, p50, p95, p99): Response duration in milliseconds for successful (2xx) transactions.
   When a trial has zero successful 2xx responses, its latency percentiles are reported as N/A.
 - client vs server counters: Benchmark measures client-observed responses; server-side request and
   connection counters are queried separately via /metrics.
"""

import argparse
import csv
import json
import os
import platform
import socket
import sys
import threading
import time
from urllib.parse import urlparse


def collect_system_metadata():
    """Gather host hardware and kernel metadata for empirical reproducibility."""
    return {
        "platform": platform.platform(),
        "system": platform.system(),
        "release": platform.release(),
        "machine": platform.machine(),
        "cpu_count": os.cpu_count() or 1,
    }


def read_http_response(sock, initial_buf=b"", timeout=2.0, return_headers=False):
    """
    Read a complete, framed HTTP/1.1 response from sock.
    Respects Content-Length framing and preserves leftover buffer bytes for pipelined requests.

    Returns:
        (status_code: int, is_close: bool, body: bytes, remaining_buf: bytes)

    Raises:
        socket.timeout: Socket operation timed out.
        ConnectionResetError / socket.error: Transport failure.
        ValueError: Malformed status line, bad headers, or premature EOF framing error.
    """
    sock.settimeout(timeout)
    buf = initial_buf

    # 1. Read until headers delimiter \r\n\r\n is found
    while b"\r\n\r\n" not in buf:
        chunk = sock.recv(4096)
        if not chunk:
            if not buf:
                raise ConnectionResetError("Connection closed before response received")
            raise ValueError("Incomplete response: connection closed before end of headers")
        buf += chunk

    hdr_end = buf.find(b"\r\n\r\n")
    header_bytes = buf[:hdr_end]
    post_header_bytes = buf[hdr_end + 4:]

    # 2. Parse status line
    lines = header_bytes.split(b"\r\n")
    if not lines or not lines[0]:
        raise ValueError("Empty HTTP response header")

    status_parts = lines[0].split(None, 2)
    if len(status_parts) < 2 or not status_parts[0].startswith(b"HTTP/"):
        raise ValueError(f"Malformed HTTP status line: {lines[0]!r}")

    try:
        status_code = int(status_parts[1])
    except ValueError:
        raise ValueError(f"Non-numeric HTTP status code: {status_parts[1]!r}")

    # 3. Parse headers for framing and connection semantics
    content_length = None
    is_close = False
    headers_dict = {}

    for line in lines[1:]:
        if b":" in line:
            k, v = line.split(b":", 1)
            k_lower = k.strip().lower()
            v_strip = v.strip().lower()
            headers_dict[k_lower.decode("latin1", errors="ignore")] = v_strip.decode("latin1", errors="ignore")
            if k_lower == b"content-length":
                try:
                    content_length = int(v_strip)
                except ValueError:
                    raise ValueError(f"Invalid Content-Length header: {v_strip!r}")
            elif k_lower == b"connection":
                if b"close" in v_strip:
                    is_close = True

    # 4. Ingest complete body per RFC framing
    if status_code in (204, 304):
        body_expected = 0
    elif content_length is not None:
        body_expected = content_length
    else:
        # Default for responses without Content-Length
        body_expected = 0

    while len(post_header_bytes) < body_expected:
        needed = body_expected - len(post_header_bytes)
        chunk = sock.recv(min(8192, max(4096, needed)))
        if not chunk:
            raise ValueError(
                f"Incomplete response: expected {body_expected} body bytes, got {len(post_header_bytes)}"
            )
        post_header_bytes += chunk

    body = post_header_bytes[:body_expected]
    remaining_buf = post_header_bytes[body_expected:]

    if return_headers:
        return status_code, is_close, body, remaining_buf, headers_dict
    return status_code, is_close, body, remaining_buf


def fetch_server_metrics(target_url):
    """
    Query server-side /metrics endpoint over raw TCP socket to fetch internal telemetry.
    Returns parsed dictionary or None if unreachable.
    """
    parsed = urlparse(target_url)
    host = parsed.hostname or "127.0.0.1"
    port = parsed.port or 8080
    req_bytes = f"GET /metrics HTTP/1.1\r\nHost: {host}:{port}\r\nConnection: close\r\n\r\n".encode("latin1")

    try:
        with socket.create_connection((host, port), timeout=2.0) as s:
            s.sendall(req_bytes)
            status, _, body, _ = read_http_response(s, timeout=2.0)
            if status == 200:
                return json.loads(body.decode("utf-8", errors="ignore"))
            elif status == 503:
                return {"qlaac_state": "OVERLOAD", "is_503": True}
    except Exception:
        pass
    return None


def run_python_load_generator(url, concurrency, duration_sec, retry_aware=False):
    """Robust concurrent HTTP load generator with explicit status-line parsing, error categorization, and optional Retry-After backoff."""
    parsed = urlparse(url)
    host = parsed.hostname or "127.0.0.1"
    port = parsed.port or 8080
    path = parsed.path or "/"

    req_bytes = (
        f"GET {path} HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"User-Agent: CorrectedBenchHarness/2.1\r\n"
        f"Connection: keep-alive\r\n\r\n"
    ).encode("latin1")

    stop_event = threading.Event()
    stats_lock = threading.Lock()

    attempted_requests = 0
    success_2xx = 0
    http_3xx = 0
    http_4xx = 0
    http_5xx = 0
    transport_errors = 0
    timeouts = 0
    invalid_responses = 0
    total_backoff_sec = 0.0
    success_latencies = []

    def worker_loop():
        nonlocal attempted_requests, success_2xx, http_3xx, http_4xx, http_5xx
        nonlocal transport_errors, timeouts, invalid_responses, total_backoff_sec

        local_attempted = 0
        local_2xx = 0
        local_3xx = 0
        local_4xx = 0
        local_5xx = 0
        local_transport_err = 0
        local_timeout_err = 0
        local_invalid_err = 0
        local_backoff_sec = 0.0
        local_latencies = []

        while not stop_event.is_set():
            sock = None
            buf = b""
            try:
                sock = socket.create_connection((host, port), timeout=2.0)
                sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

                # Up to 50 sequential requests on persistent connection
                for _ in range(50):
                    if stop_event.is_set():
                        break

                    local_attempted += 1
                    t0 = time.perf_counter()
                    sock.sendall(req_bytes)
                    if retry_aware:
                        status_code, is_close, _body, buf, hdrs = read_http_response(
                            sock, initial_buf=buf, timeout=2.0, return_headers=True
                        )
                    else:
                        status_code, is_close, _body, buf = read_http_response(
                            sock, initial_buf=buf, timeout=2.0, return_headers=False
                        )
                    t1 = time.perf_counter()

                    latency_ms = (t1 - t0) * 1000.0

                    if 200 <= status_code < 300:
                        local_2xx += 1
                        local_latencies.append(latency_ms)
                    elif 300 <= status_code < 400:
                        local_3xx += 1
                    elif 400 <= status_code < 500:
                        local_4xx += 1
                    elif 500 <= status_code < 600:
                        local_5xx += 1
                        if retry_aware:
                            ra_str = hdrs.get("retry-after") if hdrs else None
                            backoff_dur = 0.0
                            if ra_str:
                                try:
                                    backoff_dur = float(ra_str)
                                except ValueError:
                                    backoff_dur = 0.0
                            if backoff_dur <= 0.0:
                                backoff_dur = 1.0
                            t_bo_start = time.perf_counter()
                            stop_event.wait(timeout=backoff_dur)
                            local_backoff_sec += (time.perf_counter() - t_bo_start)
                    else:
                        local_invalid_err += 1

                    if is_close:
                        break

            except socket.timeout:
                local_timeout_err += 1
            except (ConnectionRefusedError, ConnectionResetError, BrokenPipeError, socket.error):
                local_transport_err += 1
            except ValueError:
                local_invalid_err += 1
            except Exception:
                local_transport_err += 1
            finally:
                if sock:
                    try:
                        sock.close()
                    except Exception:
                        pass

        with stats_lock:
            attempted_requests += local_attempted
            success_2xx += local_2xx
            http_3xx += local_3xx
            http_4xx += local_4xx
            http_5xx += local_5xx
            transport_errors += local_transport_err
            timeouts += local_timeout_err
            invalid_responses += local_invalid_err
            total_backoff_sec += local_backoff_sec
            success_latencies.extend(local_latencies)

    threads = []
    for _ in range(concurrency):
        t = threading.Thread(target=worker_loop, daemon=True)
        threads.append(t)
        t.start()

    time.sleep(duration_sec)
    stop_event.set()

    for t in threads:
        t.join(timeout=1.5)

    completed_responses = success_2xx + http_3xx + http_4xx + http_5xx
    successful_rps = success_2xx / duration_sec if duration_sec > 0 else 0.0

    # Success latency statistics: strictly describes 2xx responses
    # When zero 2xx responses succeed, percentiles are reported as N/A (not 0.00 ms)
    if success_latencies and success_2xx > 0:
        success_latencies.sort()
        count = len(success_latencies)
        avg_lat = sum(success_latencies) / count
        p50 = success_latencies[int(0.50 * (count - 1))]
        p95 = success_latencies[int(0.95 * (count - 1))]
        p99 = success_latencies[int(0.99 * (count - 1))]
        avg_lat_str = f"{avg_lat:.2f}"
        p50_str = f"{p50:.2f}"
        p95_str = f"{p95:.2f}"
        p99_str = f"{p99:.2f}"
    else:
        avg_lat = p50 = p95 = p99 = None
        avg_lat_str = p50_str = p95_str = p99_str = "N/A"

    return {
        "attempted": attempted_requests,
        "completed": completed_responses,
        "success_2xx": success_2xx,
        "http_3xx": http_3xx,
        "http_4xx": http_4xx,
        "http_5xx": http_5xx,
        "transport_errors": transport_errors,
        "timeouts": timeouts,
        "invalid_responses": invalid_responses,
        "total_backoff_sec": total_backoff_sec,
        "retry_aware": retry_aware,
        "successful_rps": successful_rps,
        "avg_lat_ms": avg_lat,
        "p50_ms": p50,
        "p95_ms": p95,
        "p99_ms": p99,
        "avg_lat_str": avg_lat_str,
        "p50_str": p50_str,
        "p95_str": p95_str,
        "p99_str": p99_str,
    }


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="HTTP/1.1 Web Server Benchmark & Experiment Orchestrator (v2.1 Refined)"
    )
    parser.add_argument(
        "--url",
        default="http://127.0.0.1:8080/health",
        help="Target URL for benchmarking (default: http://127.0.0.1:8080/health)",
    )
    parser.add_argument(
        "--clients",
        type=str,
        default="1,4,16,32",
        help="Comma-separated list of concurrent client counts",
    )
    parser.add_argument(
        "--duration",
        type=int,
        default=3,
        help="Duration in seconds for each benchmark trial",
    )
    parser.add_argument(
        "--trials",
        type=int,
        default=2,
        help="Number of repetitions per client count",
    )
    parser.add_argument(
        "--output",
        default="results/raw/benchmark_results_corrected.csv",
        help="Destination path for raw output CSV",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Validate parameters without generating load",
    )
    return parser.parse_args()


def main():
    args = parse_arguments()
    meta = collect_system_metadata()

    print("================================================================================")
    print(" HTTP/1.1 Web Server - Benchmark & Experiment Orchestrator (v2.1 Refined)")
    print("================================================================================")
    print(f" Host System:       {meta['system']} {meta['release']} ({meta['machine']})")
    print(f" Logical CPU Cores: {meta['cpu_count']}")
    print(f" Target Endpoint:   {args.url}")
    print(f" Concurrency Grid:  {args.clients}")
    print(f" Duration / Trial:  {args.duration}s (Trials per config: {args.trials})")
    print(f" Output Target:     {args.output}")
    print("--------------------------------------------------------------------------------")
    print(" METRICS ACCOUNTING & REPORTING SPECIFICATION:")
    print("  - Throughput (Successful RPS): Strictly computed as (success_2xx / duration_sec).")
    print("  - Latency Statistics: Measured exclusively for successful HTTP 2xx transactions.")
    print("    Trials with zero 2xx successes explicitly report latency percentiles as 'N/A'.")
    print("  - Overload Shedding (HTTP 503): Categorized as 5xx; kept separate from 2xx RPS.")
    print("  - Separation of Concerns: Client response totals are distinguished from server-side")
    print("    internal connection and request counters (queried from /metrics).")
    print("================================================================================")

    os.makedirs(os.path.dirname(args.output) or ".", exist_ok=True)
    csv_headers = [
        "timestamp", "url", "concurrency", "trial", "duration_sec",
        "attempted_requests", "completed_http_responses", "success_2xx",
        "http_3xx", "http_4xx", "http_5xx", "transport_errors",
        "timeouts", "invalid_responses", "successful_rps",
        "avg_latency_ms", "p50_latency_ms", "p95_latency_ms", "p99_latency_ms"
    ]

    # Check if target output exists with older incompatible schema
    if os.path.exists(args.output):
        with open(args.output, "r", encoding="utf-8", errors="ignore") as check_f:
            first_line = check_f.readline().strip()
            if "attempted_requests" not in first_line:
                print(f"[PRESERVATION] Detected legacy schema in existing file: {args.output}")
                print("               Preserving historical file and writing corrected output to:")
                args.output = "results/raw/benchmark_results_corrected.csv"
                print(f"               -> {args.output}")

    if args.dry_run:
        print("[DRY-RUN] Schema verified. Writing header template...")
        with open(args.output, "w", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(csv_headers)
        print(f"[DRY-RUN] Completed. Wrote {args.output}")
        return 0

    client_counts = [int(c.strip()) for c in args.clients.split(",") if c.strip()]
    file_exists = os.path.exists(args.output)

    with open(args.output, "a", newline="") as f:
        writer = csv.writer(f)
        if not file_exists:
            writer.writerow(csv_headers)

        print("\n| Clients | Trial | Success 2xx | 5xx Shed | 4xx Err | TransErr | Timeouts | Successful RPS | 2xx Avg Lat | 2xx p50 | 2xx p95 | 2xx p99 |")
        print("|---------|-------|-------------|----------|---------|----------|----------|----------------|-------------|---------|---------|---------|")

        for c in client_counts:
            for t in range(1, args.trials + 1):
                res = run_python_load_generator(args.url, c, args.duration)
                ts = int(time.time())

                writer.writerow([
                    ts, args.url, c, t, args.duration,
                    res["attempted"], res["completed"], res["success_2xx"],
                    res["http_3xx"], res["http_4xx"], res["http_5xx"],
                    res["transport_errors"], res["timeouts"], res["invalid_responses"],
                    f"{res['successful_rps']:.2f}",
                    res["avg_lat_str"], res["p50_str"],
                    res["p95_str"], res["p99_str"]
                ])
                f.flush()

                lat_avg_disp = f"{res['avg_lat_ms']:8.2f} ms" if res["avg_lat_ms"] is not None else "     N/A   "
                lat_p50_disp = f"{res['p50_ms']:7.2f}" if res["p50_ms"] is not None else "    N/A"
                lat_p95_disp = f"{res['p95_ms']:7.2f}" if res["p95_ms"] is not None else "    N/A"
                lat_p99_disp = f"{res['p99_ms']:7.2f}" if res["p99_ms"] is not None else "    N/A"

                print(
                    f"| {c:7d} | {t:5d} | {res['success_2xx']:11d} | {res['http_5xx']:8d} | "
                    f"{res['http_4xx']:7d} | {res['transport_errors']:8d} | {res['timeouts']:8d} | "
                    f"{res['successful_rps']:14.1f} | {lat_avg_disp} | "
                    f"{lat_p50_disp} | {lat_p95_disp} | {lat_p99_disp} |"
                )
                time.sleep(0.5)

            # Query and display server-side internal counters after trial batch
            srv = fetch_server_metrics(args.url)
            if srv:
                print(
                    f"   └─ [SERVER METRICS] Conns(Acc:{srv.get('total_connections_accepted', 0)}, "
                    f"Rej:{srv.get('total_connections_rejected', 0)}) | "
                    f"Reqs Completed:{srv.get('total_requests_completed', 0)} | "
                    f"503 Overload Sent:{srv.get('total_overload_503_sent', 0)} | "
                    f"QLAAC State:{srv.get('qlaac_state', 'N/A')} (Trans:{srv.get('qlaac_overload_transitions', 0)}, "
                    f"Shed:{srv.get('qlaac_shed_requests', 0)})"
                )

    print("\n[SUCCESS] Benchmark run complete. Results recorded to:", args.output)
    return 0


if __name__ == "__main__":
    sys.exit(main())
