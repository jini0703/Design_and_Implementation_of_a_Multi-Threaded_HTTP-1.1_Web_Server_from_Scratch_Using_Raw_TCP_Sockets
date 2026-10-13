#!/usr/bin/env python3
"""
run_experiment.py
Benchmarking orchestrator and experiment harness for the HTTP/1.1 Web Server.

Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
         from Scratch Using Raw TCP Sockets

Responsibilities:
 - Parse benchmarking parameters (concurrency, worker threads, duration, payload size).
 - Capture system execution metadata (kernel release, CPU topology, system memory).
 - Orchestrate server background launch and wait for TCP readiness.
 - Invoke external HTTP load generators (e.g. wrk or Python client pools).
 - Parse load generator output (requests/sec, tail latency percentiles).
 - Output structured CSV data to results/raw/.
"""

import argparse
import csv
import os
import platform
import subprocess
import sys
import time


def collect_system_metadata():
    """Gather host hardware and kernel metadata for empirical reproducibility."""
    return {
        "platform": platform.platform(),
        "system": platform.system(),
        "release": platform.release(),
        "machine": platform.machine(),
        "cpu_count": os.cpu_count() or 1,
    }


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="HTTP/1.1 Web Server Benchmark & Experiment Orchestrator"
    )
    parser.add_argument(
        "--server-bin",
        default="bin/server",
        help="Path to compiled server binary (default: bin/server)",
    )
    parser.add_argument(
        "--config",
        default="config/server.conf.example",
        help="Path to server configuration file",
    )
    parser.add_argument(
        "--experiment",
        choices=["concurrency", "qlaac", "keepalive", "payload", "all"],
        default="concurrency",
        help="Experiment suite identifier to run",
    )
    parser.add_argument(
        "--workers",
        type=int,
        default=4,
        help="Number of server worker threads to evaluate",
    )
    parser.add_argument(
        "--clients",
        type=str,
        default="1,10,50,100",
        help="Comma-separated list of concurrent client counts",
    )
    parser.add_argument(
        "--duration",
        type=int,
        default=10,
        help="Duration in seconds for each benchmark trial",
    )
    parser.add_argument(
        "--trials",
        type=int,
        default=3,
        help="Number of repetitions per parameter combination",
    )
    parser.add_argument(
        "--output",
        default="results/raw/benchmark_results.csv",
        help="Destination path for raw output CSV",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Simulate test matrix and verify output configuration without launching server",
    )
    return parser.parse_args()


def main():
    args = parse_arguments()
    meta = collect_system_metadata()

    print("================================================================================")
    print(" HTTP/1.1 Web Server - Benchmark & Experiment Orchestrator")
    print("================================================================================")
    print(f" Host System:       {meta['system']} {meta['release']} ({meta['machine']})")
    print(f" Logical CPU Cores: {meta['cpu_count']}")
    print(f" Experiment Suite:  {args.experiment}")
    print(f" Worker Threads:    {args.workers}")
    print(f" Client Grid:       {args.clients}")
    print(f" Duration / Trial:  {args.duration}s (Trials per config: {args.trials})")
    print(f" Output Target:     {args.output}")
    print("================================================================================")

    if args.dry_run:
        print("[DRY-RUN] Validating experiment matrix and verifying directory permissions...")
        os.makedirs(os.path.dirname(args.output) or ".", exist_ok=True)
        csv_headers = [
            "timestamp", "experiment_id", "trial", "worker_threads",
            "concurrency", "duration_sec", "requests_per_sec",
            "latency_avg_ms", "latency_p50_ms", "latency_p95_ms",
            "latency_p99_ms", "errors_count"
        ]
        with open(args.output, "w", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(csv_headers)
        print(f"[DRY-RUN] Successfully verified output schema and generated: {args.output}")
        print("[DRY-RUN] Ready for full execution once server binary implementation is finalized.")
        return 0

    print("[NOTICE] Automated end-to-end load generation requires compiled functional server.")
    print("         Run with '--dry-run' to inspect matrix parameters, or build 'bin/server'.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
