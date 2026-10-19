#!/usr/bin/env python3
"""
diagnose_trial.py
Diagnostic tool: runs a benchmark trial with high-frequency (100ms) background sampling
of server-side /metrics to capture controller transitions, queue depth, queue wait,
and rejection counts throughout the trial timeline.
"""

import json
import os
import signal
import socket
import subprocess
import sys
import threading
import time
from urllib.parse import urlparse

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if PROJECT_ROOT not in sys.path:
    sys.path.insert(0, PROJECT_ROOT)

from benchmark.run_experiment import run_python_load_generator, fetch_server_metrics


def sample_metrics_timeline(url, duration_sec, interval_sec=0.2):
    timeline = []
    stop_event = threading.Event()

    def sampler_loop():
        t0 = time.time()
        while not stop_event.is_set():
            t_now = time.time() - t0
            metrics = fetch_server_metrics(url)
            if metrics:
                timeline.append({
                    "time_sec": round(t_now, 3),
                    "qlaac_state": metrics.get("qlaac_state", "UNKNOWN"),
                    "active_workers": metrics.get("active_workers", 0),
                    "queue_depth": metrics.get("current_queue_depth", 0),
                    "p95_proc_ms": metrics.get("p95_latency_ms", 0.0),
                    "p95_qwait_ms": metrics.get("p95_queue_wait_ms", 0.0),
                    "reqs_completed": metrics.get("total_requests_completed", 0),
                    "conns_accepted": metrics.get("total_connections_accepted", 0),
                    "conns_rejected": metrics.get("total_connections_rejected", 0),
                    "qlaac_shed": metrics.get("qlaac_shed_requests", 0),
                    "overload_503": metrics.get("total_overload_503_sent", 0),
                    "overload_trans": metrics.get("qlaac_overload_transitions", 0),
                })
            time.sleep(interval_sec)

    th = threading.Thread(target=sampler_loop, daemon=True)
    th.start()

    def stop():
        stop_event.set()
        th.join(timeout=1.0)
        return timeline

    return stop


def run_diagnostic_trial(mode, concurrency, duration_sec=10, port=8080):
    server_bin = os.path.join(PROJECT_ROOT, "bin", "server")
    conf_file = os.path.join(PROJECT_ROOT, "config", "server.conf.example")
    url = f"http://127.0.0.1:{port}/health"

    cmd = [server_bin, "-c", conf_file, "-p", str(port)]
    if mode.lower() == "qlaac":
        cmd.append("-a")

    print(f"\n[DIAGNOSTIC] Starting fresh server ({mode.upper()}, C={concurrency}, {duration_sec}s)...")
    proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
                            preexec_fn=os.setsid if hasattr(os, "setsid") else None)

    # Wait for server
    time.sleep(0.4)

    # Start timeline sampler
    stop_sampler = sample_metrics_timeline(f"http://127.0.0.1:{port}/metrics", duration_sec, interval_sec=0.25)

    print(f"[DIAGNOSTIC] Running {concurrency}-client load generator for {duration_sec}s...")
    bench_result = run_python_load_generator(url, concurrency, duration_sec)

    timeline = stop_sampler()

    # Terminate server
    try:
        if hasattr(os, "killpg") and hasattr(os, "getpgid"):
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        else:
            proc.terminate()
        proc.wait(timeout=2.0)
    except Exception:
        proc.kill()
        proc.wait()

    server_stderr = proc.stderr.read().decode("utf-8", errors="ignore") if proc.stderr else ""

    print("\n--- SERVER LOGGED TRANSITIONS ---")
    for line in server_stderr.splitlines():
        if "[QLAAC]" in line or "Transition" in line or "Recovery" in line:
            print(f"  {line}")

    print("\n--- TIMELINE SAMPLES (every ~250ms) ---")
    print(f"{'Time':>6} | {'State':>8} | {'ActW':>4} | {'Queue':>5} | {'p95 QWait':>9} | {'p95 Proc':>8} | {'2xx Done':>8} | {'503 Sent':>8} | {'Shed':>6}")
    print("-" * 80)
    for s in timeline:
        print(f"{s['time_sec']:6.2f} | {s['qlaac_state']:>8} | {s['active_workers']:4d} | {s['queue_depth']:5d} | {s['p95_qwait_ms']:8.2f}m | {s['p95_proc_ms']:7.2f}m | {s['reqs_completed']:8d} | {s['overload_503']:8d} | {s['qlaac_shed']:6d}")

    print("\n--- CLIENT BENCHMARK RESULT ---")
    print(f"2xx Success: {bench_result['success_2xx']} ({bench_result['successful_rps']:.1f} RPS)")
    print(f"5xx Shed:    {bench_result['http_5xx']}")
    print(f"Timeouts:    {bench_result['timeouts']}")
    print(f"Trans Errors:{bench_result['transport_errors']}")
    print(f"Latency:     avg={bench_result['avg_lat_str']} ms, p50={bench_result['p50_str']} ms, p95={bench_result['p95_str']} ms, p99={bench_result['p99_str']} ms")
    print("=" * 80)


if __name__ == "__main__":
    mode = sys.argv[1] if len(sys.argv) > 1 else "qlaac"
    c = int(sys.argv[2]) if len(sys.argv) > 2 else 8
    dur = int(sys.argv[3]) if len(sys.argv) > 3 else 5
    run_diagnostic_trial(mode, c, dur)
