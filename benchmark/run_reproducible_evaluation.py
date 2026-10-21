#!/usr/bin/env python3
"""
run_reproducible_evaluation.py
Automated reproducible benchmarking orchestrator with per-trial server process restarts.

Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
         from Scratch Using Raw TCP Sockets

Key Capabilities:
 - Uses the exact same binary build and load generator methodology as run_experiment.py.
 - Restarts the server binary between consecutive trials to eliminate controller state carryover.
 - Evaluates concurrency levels [2, 8, 32, 64] across 5 trials of 10 seconds each.
 - Explicitly distinguishes baseline (fixed-policy) and QLAAC (adaptive control) modes.
 - Outputs clean, directly comparable CSV files preserving historical benchmark runs.
"""

import argparse
import csv
import http.client
import json
import os
import signal
import socket
import subprocess
import sys
import threading
import time
from urllib.parse import urlparse

# Ensure project root is in sys.path
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if PROJECT_ROOT not in sys.path:
    sys.path.insert(0, PROJECT_ROOT)

from benchmark.run_experiment import run_python_load_generator, fetch_server_metrics, collect_system_metadata


def wait_for_server(host, port, max_wait_sec=5.0):
    """
    Poll until the expected HTTP server is fully operational and verified.

    Sends an HTTP GET request to /health and verifies:
      1. HTTP status code is 200 OK.
      2. The response body is valid JSON.
      3. The JSON explicitly contains "status": "healthy" (no default allowed).
      4. The JSON identifies the server as 'HTTP-Server-C17/1.0.0'
         and protocol as 'HTTP/1.1'.

    Retries with monotonic deadline backoff until max_wait_sec has elapsed.
    Safely handles connection refusal, timeouts, transport errors,
    HTTP error codes, malformed JSON, and unrelated listeners on the port.
    """
    start = time.monotonic()
    deadline = start + max_wait_sec
    port_int = int(port)
    host_hdr = f"[{host}]:{port_int}" if ":" in host and not host.startswith("[") else f"{host}:{port_int}"

    while True:
        now = time.monotonic()
        remaining = deadline - now
        if remaining <= 0:
            break

        # Bound per-attempt network timeout so it never exceeds the remaining deadline
        attempt_timeout = min(remaining, 1.0)
        conn = None
        try:
            conn = http.client.HTTPConnection(host, port_int, timeout=attempt_timeout)
            conn.request(
                "GET",
                "/health",
                headers={
                    "Host": host_hdr,
                    "Connection": "close",
                    "User-Agent": "ReadinessProbe/1.0",
                },
            )
            resp = conn.getresponse()
            if resp.status == 200:
                raw_body = resp.read(65536)
                data = json.loads(raw_body.decode("utf-8", errors="replace"))
                if (
                    isinstance(data, dict)
                    and data.get("status") == "healthy"
                    and data.get("server") == "HTTP-Server-C17/1.0.0"
                    and data.get("protocol") == "HTTP/1.1"
                ):
                    return True
        except (OSError, http.client.HTTPException, json.JSONDecodeError, ValueError):
            pass
        finally:
            if conn:
                try:
                    conn.close()
                except OSError:
                    pass

        # Brief pause before retrying, capped to avoid oversleeping past deadline
        now_after = time.monotonic()
        remaining_after = deadline - now_after
        if remaining_after <= 0:
            break
        time.sleep(min(0.05, remaining_after))

    return False


def run_evaluation(mode, output_csv, timeline_csv=None, port=8080, duration=10, trials=5,
                   concurrencies=[2, 8, 32, 64], retry_aware=False):
    server_bin = os.path.join(PROJECT_ROOT, "bin", "server")
    conf_file = os.path.join(PROJECT_ROOT, "config", "server.conf.example")
    url = f"http://127.0.0.1:{port}/health"
    metrics_url = f"http://127.0.0.1:{port}/metrics"

    if not os.path.exists(server_bin):
        raise FileNotFoundError(f"Server binary not found at {server_bin}. Run 'make' first.")

    os.makedirs(os.path.dirname(output_csv) or ".", exist_ok=True)
    if timeline_csv:
        os.makedirs(os.path.dirname(timeline_csv) or ".", exist_ok=True)

    if retry_aware:
        csv_headers = [
            "timestamp", "url", "concurrency", "trial", "duration_sec",
            "attempted_requests", "completed_http_responses", "success_2xx",
            "http_3xx", "http_4xx", "http_5xx", "transport_errors",
            "timeouts", "invalid_responses", "total_backoff_sec", "successful_rps",
            "avg_latency_ms", "p50_latency_ms", "p95_latency_ms", "p99_latency_ms"
        ]
    else:
        csv_headers = [
            "timestamp", "url", "concurrency", "trial", "duration_sec",
            "attempted_requests", "completed_http_responses", "success_2xx",
            "http_3xx", "http_4xx", "http_5xx", "transport_errors",
            "timeouts", "invalid_responses", "successful_rps",
            "avg_latency_ms", "p50_latency_ms", "p95_latency_ms", "p99_latency_ms"
        ]

    timeline_headers = [
        "timestamp", "mode", "concurrency", "trial", "elapsed_sec",
        "qlaac_state", "active_workers", "total_workers", "queue_depth", "queue_capacity",
        "p50_qwait_ms", "p95_qwait_ms", "p99_qwait_ms",
        "p50_proc_ms", "p95_proc_ms", "p99_proc_ms",
        "reqs_completed", "status_2xx", "status_5xx",
        "conns_accepted", "conns_rejected", "overload_503_sent",
        "qlaac_shed_requests", "overload_transitions"
    ]

    timeline_file = None
    timeline_writer = None
    if timeline_csv:
        timeline_file = open(timeline_csv, "w", newline="", encoding="utf-8")
        timeline_writer = csv.writer(timeline_file)
        timeline_writer.writerow(timeline_headers)

    try:
        with open(output_csv, "w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(csv_headers)

            meta = collect_system_metadata()
            client_label = "RETRY-AWARE" if retry_aware else "STANDARD"
            print("=" * 95)
            print(f" REPRODUCIBLE EVALUATION: {mode.upper()} MODE [{client_label}] (Per-Trial Process Isolation)")
            print("=" * 95)
            print(f" Host:         {meta['system']} {meta['release']} ({meta['machine']}) - {meta['cpu_count']} cores")
            print(f" Binary:       {server_bin}")
            print(f" Config:       {conf_file}")
            print(f" Target URL:   {url}")
            print(f" Concurrency:  {concurrencies}")
            print(f" Trials:       {trials} trials x {duration}s per condition")
            print(f" Client Mode:  {client_label}")
            print(f" Output CSV:   {output_csv}")
            if timeline_csv:
                print(f" Timeline CSV: {timeline_csv}")
            print("-" * 95)
            print("| Clients | Trial | Success 2xx | 5xx Shed | TransErr | Timeouts | Successful RPS | 2xx Avg Lat | 2xx p50 | 2xx p95 | 2xx p99 |")
            print("|---------|-------|-------------|----------|----------|----------|----------------|-------------|---------|---------|---------|")

            for c in concurrencies:
                for t in range(1, trials + 1):
                    # 1. Start fresh server process for this trial
                    cmd = [server_bin, "-c", conf_file, "-p", str(port)]
                    if mode.lower() == "qlaac":
                        cmd.append("-a")

                    proc = subprocess.Popen(
                        cmd,
                        stdout=subprocess.DEVNULL,
                        stderr=subprocess.PIPE,
                        preexec_fn=os.setsid if hasattr(os, "setsid") else None
                    )

                    stop_timeline = threading.Event()
                    sampler_thread = None
                    last_snap = {}

                    def timeline_sampler_worker():
                        nonlocal last_snap
                        t_start = time.time()
                        while not stop_timeline.is_set():
                            t_curr = time.time()
                            el = round(t_curr - t_start, 3)
                            snap = fetch_server_metrics(metrics_url)
                            if snap and timeline_writer:
                                is_503 = snap.get("is_503", False)
                                if not is_503:
                                    last_snap = snap
                                st = "OVERLOAD" if is_503 else snap.get("qlaac_state", "NORMAL")
                                timeline_writer.writerow([
                                    int(t_curr), mode.lower(), c, t, el,
                                    st,
                                    last_snap.get("active_workers", 4 if is_503 else 0),
                                    last_snap.get("total_workers", 4),
                                    last_snap.get("current_queue_depth", 0),
                                    64,
                                    last_snap.get("median_queue_wait_ms", 0.0),
                                    last_snap.get("p95_queue_wait_ms", 0.0),
                                    last_snap.get("p99_queue_wait_ms", 0.0),
                                    last_snap.get("median_latency_ms", 0.0),
                                    last_snap.get("p95_latency_ms", 0.0),
                                    last_snap.get("p99_latency_ms", 0.0),
                                    last_snap.get("total_requests_completed", 0),
                                    last_snap.get("status_2xx_count", 0),
                                    last_snap.get("status_5xx_count", 0),
                                    last_snap.get("total_connections_accepted", 0),
                                    last_snap.get("total_connections_rejected", 0),
                                    last_snap.get("total_overload_503_sent", 0),
                                    last_snap.get("qlaac_shed_requests", 0),
                                    last_snap.get("qlaac_overload_transitions", 0),
                                ])
                                if timeline_file:
                                    timeline_file.flush()
                            time.sleep(0.25)

                    try:
                        if not wait_for_server("127.0.0.1", port, max_wait_sec=5.0):
                            raise RuntimeError(f"Server failed to start on port {port}")

                        # 2. Brief warm-up (1s, 2 clients)
                        run_python_load_generator(url, 2, 1.0, retry_aware=retry_aware)
                        time.sleep(0.2)

                        # Start background periodic timeline sampler for this trial
                        if timeline_writer:
                            sampler_thread = threading.Thread(target=timeline_sampler_worker, daemon=True)
                            sampler_thread.start()

                        # 3. Timed trial measurement
                        res = run_python_load_generator(url, c, duration, retry_aware=retry_aware)
                        ts = int(time.time())

                        # Stop timeline sampler
                        stop_timeline.set()
                        if sampler_thread:
                            sampler_thread.join(timeout=1.0)

                        # 4. Write CSV row
                        if retry_aware:
                            writer.writerow([
                                ts, url, c, t, duration,
                                res["attempted"], res["completed"], res["success_2xx"],
                                res["http_3xx"], res["http_4xx"], res["http_5xx"],
                                res["transport_errors"], res["timeouts"], res["invalid_responses"],
                                f"{res['total_backoff_sec']:.2f}",
                                f"{res['successful_rps']:.2f}",
                                res["avg_lat_str"], res["p50_str"],
                                res["p95_str"], res["p99_str"]
                            ])
                        else:
                            writer.writerow([
                                ts, url, c, t, duration,
                                res["attempted"], res["completed"], res["success_2xx"],
                                res["http_3xx"], res["http_4xx"], res["http_5xx"],
                                res["transport_errors"], res["timeouts"], res["invalid_responses"],
                                f"{res['successful_rps']:.2f}",
                                res["avg_lat_str"], res["p50_str"],
                                res["p95_str"], res["p99_str"]
                            ])
                        f.flush()

                        # 5. Query final telemetry before shutdown
                        srv = fetch_server_metrics(url)
                        srv_info = ""
                        if srv:
                            srv_info = f" [State: {srv.get('qlaac_state', 'N/A')}, Shed: {srv.get('qlaac_shed_requests', 0)}]"

                        # 6. Display row
                        lat_avg_disp = f"{res['avg_lat_ms']:8.2f} ms" if res["avg_lat_ms"] is not None else "     N/A   "
                        lat_p50_disp = f"{res['p50_ms']:7.2f}" if res["p50_ms"] is not None else "    N/A"
                        lat_p95_disp = f"{res['p95_ms']:7.2f}" if res["p95_ms"] is not None else "    N/A"
                        lat_p99_disp = f"{res['p99_ms']:7.2f}" if res["p99_ms"] is not None else "    N/A"

                        print(
                            f"| {c:7d} | {t:5d} | {res['success_2xx']:11d} | {res['http_5xx']:8d} | "
                            f"{res['transport_errors']:8d} | {res['timeouts']:8d} | "
                            f"{res['successful_rps']:14.1f} | {lat_avg_disp} | "
                            f"{lat_p50_disp} | {lat_p95_disp} | {lat_p99_disp} |{srv_info}"
                        )

                    finally:
                        stop_timeline.set()
                        # Terminate server cleanly
                        try:
                            if hasattr(os, "killpg") and hasattr(os, "getpgid"):
                                os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
                            else:
                                proc.terminate()
                            proc.wait(timeout=3.0)
                        except Exception:
                            proc.kill()
                            proc.wait()
                        finally:
                            if proc.stderr:
                                proc.stderr.close()

                        # Small cool-down to ensure port release
                        time.sleep(0.3)

        print("-" * 95)
        print(f"[COMPLETE] {mode.upper()} evaluation finished. Raw CSV saved to: {output_csv}\n")
    finally:
        if timeline_file:
            timeline_file.close()


def run_paired_evaluation(baseline_csv, qlaac_csv, baseline_timeline=None, qlaac_timeline=None,
                          port=8080, duration=10, trials=5, concurrencies=[2, 8, 32, 64],
                          retry_aware=False, seed=42):
    """
    Execute matched paired/interleaved evaluation between Baseline and QLAAC.
    For each matched condition (c, t), alternate which configuration executes first
    to eliminate sequential execution order, clock-frequency scaling, and thermal bias.
    """
    server_bin = os.path.join(PROJECT_ROOT, "bin", "server")
    conf_file = os.path.join(PROJECT_ROOT, "config", "server.conf.example")
    url = f"http://127.0.0.1:{port}/health"
    metrics_url = f"http://127.0.0.1:{port}/metrics"

    if not os.path.exists(server_bin):
        raise FileNotFoundError(f"Server binary not found at {server_bin}. Run 'make' first.")

    os.makedirs(os.path.dirname(baseline_csv) or ".", exist_ok=True)
    os.makedirs(os.path.dirname(qlaac_csv) or ".", exist_ok=True)
    if baseline_timeline:
        os.makedirs(os.path.dirname(baseline_timeline) or ".", exist_ok=True)
    if qlaac_timeline:
        os.makedirs(os.path.dirname(qlaac_timeline) or ".", exist_ok=True)

    if retry_aware:
        csv_headers = [
            "timestamp", "url", "concurrency", "trial", "duration_sec",
            "attempted_requests", "completed_http_responses", "success_2xx",
            "http_3xx", "http_4xx", "http_5xx", "transport_errors",
            "timeouts", "invalid_responses", "total_backoff_sec", "successful_rps",
            "avg_latency_ms", "p50_latency_ms", "p95_latency_ms", "p99_latency_ms"
        ]
    else:
        csv_headers = [
            "timestamp", "url", "concurrency", "trial", "duration_sec",
            "attempted_requests", "completed_http_responses", "success_2xx",
            "http_3xx", "http_4xx", "http_5xx", "transport_errors",
            "timeouts", "invalid_responses", "successful_rps",
            "avg_latency_ms", "p50_latency_ms", "p95_latency_ms", "p99_latency_ms"
        ]

    timeline_headers = [
        "timestamp", "mode", "concurrency", "trial", "elapsed_sec",
        "qlaac_state", "active_workers", "total_workers", "queue_depth", "queue_capacity",
        "p50_qwait_ms", "p95_qwait_ms", "p99_qwait_ms",
        "p50_proc_ms", "p95_proc_ms", "p99_proc_ms",
        "reqs_completed", "status_2xx", "status_5xx",
        "conns_accepted", "conns_rejected", "overload_503_sent",
        "qlaac_shed_requests", "overload_transitions"
    ]

    meta = collect_system_metadata()
    client_label = "RETRY-AWARE" if retry_aware else "STANDARD"

    print("=" * 105)
    print(f" PAIRED / INTERLEAVED EVALUATION [{client_label}] (Matched Baseline vs QLAAC)")
    print("=" * 105)
    print(f" Host:         {meta['system']} {meta['release']} ({meta['machine']}) - {meta['cpu_count']} cores")
    print(f" Binary:       {server_bin}")
    print(f" Config:       {conf_file}")
    print(f" Target URL:   {url}")
    print(f" Concurrency:  {concurrencies}")
    print(f" Trials:       {trials} matched pairs x {duration}s per condition (Alternating Execution Order)")
    print(f" Baseline CSV: {baseline_csv}")
    print(f" QLAAC CSV:    {qlaac_csv}")
    print("-" * 105)
    print("| Mode     | Clients | Trial | Success 2xx | 5xx Shed | TransErr | Timeouts | Successful RPS | 2xx Avg Lat | 2xx p50 | 2xx p95 | 2xx p99 |")
    print("|----------|---------|-------|-------------|----------|----------|----------|----------------|-------------|---------|---------|---------|")

    f_b = open(baseline_csv, "w", newline="", encoding="utf-8")
    b_writer = csv.writer(f_b)
    b_writer.writerow(csv_headers)

    f_q = open(qlaac_csv, "w", newline="", encoding="utf-8")
    q_writer = csv.writer(f_q)
    q_writer.writerow(csv_headers)

    b_tl_file = open(baseline_timeline, "w", newline="", encoding="utf-8") if baseline_timeline else None
    b_tl_writer = csv.writer(b_tl_file) if b_tl_file else None
    if b_tl_writer:
        b_tl_writer.writerow(timeline_headers)

    q_tl_file = open(qlaac_timeline, "w", newline="", encoding="utf-8") if qlaac_timeline else None
    q_tl_writer = csv.writer(q_tl_file) if q_tl_file else None
    if q_tl_writer:
        q_tl_writer.writerow(timeline_headers)

    def execute_single_trial(mode, c, t, writer, f_csv, tl_writer, tl_file):
        cmd = [server_bin, "-c", conf_file, "-p", str(port)]
        if mode == "qlaac":
            cmd.append("-a")

        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
            preexec_fn=os.setsid if hasattr(os, "setsid") else None
        )

        stop_timeline = threading.Event()
        sampler_thread = None
        last_snap = {}

        def timeline_sampler_worker():
            nonlocal last_snap
            t_start = time.time()
            while not stop_timeline.is_set():
                t_curr = time.time()
                el = round(t_curr - t_start, 3)
                snap = fetch_server_metrics(metrics_url)
                if snap and tl_writer:
                    is_503 = snap.get("is_503", False)
                    if not is_503:
                        last_snap = snap
                    st = "OVERLOAD" if is_503 else snap.get("qlaac_state", "NORMAL")
                    tl_writer.writerow([
                        int(t_curr), mode.lower(), c, t, el,
                        st,
                        last_snap.get("active_workers", 4 if is_503 else 0),
                        last_snap.get("total_workers", 4),
                        last_snap.get("current_queue_depth", 0),
                        64,
                        last_snap.get("median_queue_wait_ms", 0.0),
                        last_snap.get("p95_queue_wait_ms", 0.0),
                        last_snap.get("p99_queue_wait_ms", 0.0),
                        last_snap.get("median_latency_ms", 0.0),
                        last_snap.get("p95_latency_ms", 0.0),
                        last_snap.get("p99_latency_ms", 0.0),
                        last_snap.get("total_requests_completed", 0),
                        last_snap.get("total_requests_2xx", 0),
                        last_snap.get("total_requests_5xx", 0),
                        last_snap.get("total_connections_accepted", 0),
                        last_snap.get("total_connections_rejected", 0),
                        last_snap.get("total_overload_503_sent", 0),
                        last_snap.get("qlaac_shed_requests", 0),
                        last_snap.get("qlaac_overload_transitions", 0),
                    ])
                    if tl_file:
                        tl_file.flush()
                time.sleep(0.25)

        try:
            if not wait_for_server("127.0.0.1", port, max_wait_sec=5.0):
                raise RuntimeError(f"Server failed to start on port {port}")

            # 2. Warm-up (1s, 2 clients)
            run_python_load_generator(url, 2, 1.0, retry_aware=retry_aware)
            time.sleep(0.2)

            if tl_writer:
                sampler_thread = threading.Thread(target=timeline_sampler_worker, daemon=True)
                sampler_thread.start()

            # 3. Timed trial measurement
            res = run_python_load_generator(url, c, duration, retry_aware=retry_aware)
            ts = int(time.time())

            stop_timeline.set()
            if sampler_thread:
                sampler_thread.join(timeout=1.0)

            # 4. Write CSV row
            if retry_aware:
                writer.writerow([
                    ts, url, c, t, duration,
                    res["attempted"], res["completed"], res["success_2xx"],
                    res["http_3xx"], res["http_4xx"], res["http_5xx"],
                    res["transport_errors"], res["timeouts"], res["invalid_responses"],
                    f"{res['total_backoff_sec']:.2f}",
                    f"{res['successful_rps']:.2f}",
                    res["avg_lat_str"], res["p50_str"],
                    res["p95_str"], res["p99_str"]
                ])
            else:
                writer.writerow([
                    ts, url, c, t, duration,
                    res["attempted"], res["completed"], res["success_2xx"],
                    res["http_3xx"], res["http_4xx"], res["http_5xx"],
                    res["transport_errors"], res["timeouts"], res["invalid_responses"],
                    f"{res['successful_rps']:.2f}",
                    res["avg_lat_str"], res["p50_str"],
                    res["p95_str"], res["p99_str"]
                ])
            f_csv.flush()

            # 5. Query final telemetry before shutdown
            srv = fetch_server_metrics(url)
            srv_info = ""
            if srv:
                srv_info = f" [State: {srv.get('qlaac_state', 'N/A')}, Shed: {srv.get('qlaac_shed_requests', 0)}]"

            # 6. Display row
            lat_avg_disp = f"{res['avg_lat_ms']:8.2f} ms" if res["avg_lat_ms"] is not None else "     N/A   "
            lat_p50_disp = f"{res['p50_ms']:7.2f}" if res["p50_ms"] is not None else "    N/A"
            lat_p95_disp = f"{res['p95_ms']:7.2f}" if res["p95_ms"] is not None else "    N/A"
            lat_p99_disp = f"{res['p99_ms']:7.2f}" if res["p99_ms"] is not None else "    N/A"

            print(
                f"| {mode.upper():8s} | {c:7d} | {t:5d} | {res['success_2xx']:11d} | {res['http_5xx']:8d} | "
                f"{res['transport_errors']:8d} | {res['timeouts']:8d} | "
                f"{res['successful_rps']:14.1f} | {lat_avg_disp} | "
                f"{lat_p50_disp} | {lat_p95_disp} | {lat_p99_disp} |{srv_info}"
            )

        finally:
            stop_timeline.set()
            try:
                if hasattr(os, "killpg") and hasattr(os, "getpgid"):
                    os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
                else:
                    proc.terminate()
                proc.wait(timeout=3.0)
            except Exception:
                proc.kill()
                proc.wait()
            finally:
                if proc.stderr:
                    proc.stderr.close()

            time.sleep(0.5)

    try:
        for c in concurrencies:
            for t in range(1, trials + 1):
                # Alternating execution order within each matched pair
                if t % 2 == 1:
                    order = ["baseline", "qlaac"]
                else:
                    order = ["qlaac", "baseline"]

                for m in order:
                    if m == "baseline":
                        execute_single_trial("baseline", c, t, b_writer, f_b, b_tl_writer, b_tl_file)
                    else:
                        execute_single_trial("qlaac", c, t, q_writer, f_q, q_tl_writer, q_tl_file)

        print("-" * 105)
        print(f"[COMPLETE] Paired evaluation finished for [{client_label}].")
        print(f"  Baseline CSV: {baseline_csv}")
        print(f"  QLAAC CSV:    {qlaac_csv}\n")
    finally:
        f_b.close()
        f_q.close()
        if b_tl_file:
            b_tl_file.close()
        if q_tl_file:
            q_tl_file.close()


def main():
    parser = argparse.ArgumentParser(description="Reproducible benchmark harness with per-trial process isolation.")
    parser.add_argument("--mode", choices=["baseline", "qlaac", "both"], default="both",
                        help="Operating mode to benchmark (default: both)")
    parser.add_argument("--client-mode", choices=["standard", "retry-aware", "both"], default="standard",
                        help="Client workload mode: standard or retry-aware (default: standard)")
    parser.add_argument("--paired", action="store_true", default=True,
                        help="Use matched interleaved execution to eliminate run-order bias (default: True)")
    parser.add_argument("--sequential", action="store_true", default=False,
                        help="Run sequential blocks instead of interleaved pairs")
    parser.add_argument("--port", type=int, default=8080, help="Server port (default: 8080)")
    parser.add_argument("--duration", type=int, default=10, help="Trial duration in seconds (default: 10)")
    parser.add_argument("--trials", type=int, default=5, help="Trials per concurrency condition (default: 5)")
    parser.add_argument("--tag", default="v4_paired_20261019", help="Tag for output filenames (default: v4_paired_20261019)")
    args = parser.parse_args()

    client_modes = ["standard", "retry-aware"] if args.client_mode == "both" else [args.client_mode]

    for cm in client_modes:
        is_retry = (cm == "retry-aware")
        tag_suffix = f"{cm}_{args.tag}"

        baseline_csv = os.path.join(PROJECT_ROOT, "results", "raw", f"experiment_baseline_{tag_suffix}.csv")
        qlaac_csv = os.path.join(PROJECT_ROOT, "results", "raw", f"experiment_qlaac_{tag_suffix}.csv")
        baseline_timeline = os.path.join(PROJECT_ROOT, "results", "raw", f"diagnostic_timeline_baseline_{tag_suffix}.csv")
        qlaac_timeline = os.path.join(PROJECT_ROOT, "results", "raw", f"diagnostic_timeline_qlaac_{tag_suffix}.csv")

        if args.paired and not args.sequential and args.mode == "both":
            run_paired_evaluation(baseline_csv, qlaac_csv,
                                  baseline_timeline=baseline_timeline,
                                  qlaac_timeline=qlaac_timeline,
                                  port=args.port, duration=args.duration,
                                  trials=args.trials, retry_aware=is_retry)
        else:
            if args.mode in ("baseline", "both"):
                run_evaluation("baseline", baseline_csv, timeline_csv=baseline_timeline,
                               port=args.port, duration=args.duration, trials=args.trials, retry_aware=is_retry)

            if args.mode in ("qlaac", "both"):
                run_evaluation("qlaac", qlaac_csv, timeline_csv=qlaac_timeline,
                               port=args.port, duration=args.duration, trials=args.trials, retry_aware=is_retry)


if __name__ == "__main__":
    main()
