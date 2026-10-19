"""
verify_benchmark_statistics.py
Single Source of Truth script for computing all means, sample standard deviations,
latencies, 5xx counts, and shedding rates directly from:
- results/raw/experiment_baseline_tuned_comparison.csv
- results/raw/experiment_qlaac_tuned_comparison.csv
"""

import csv
import statistics
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent.parent
BASELINE_CSV = BASE_DIR / "results" / "raw" / "experiment_baseline_tuned_comparison.csv"
QLAAC_CSV = BASE_DIR / "results" / "raw" / "experiment_qlaac_tuned_comparison.csv"

def compute_metrics(csv_path):
    with open(csv_path, newline='', encoding='utf-8') as f:
        rows = list(csv.DictReader(f))

    by_c = {}
    for r in rows:
        c = int(r['concurrency'])
        by_c.setdefault(c, []).append(r)

    results = {}
    for c in sorted(by_c.keys()):
        trials = by_c[c]
        n = len(trials)
        assert n == 5, f"Expected 5 trials for C={c}, found {n}"

        rps_list = [float(t['successful_rps']) for t in trials]
        p50_list = [float(t['p50_latency_ms']) for t in trials if t['p50_latency_ms'] != 'N/A']
        p95_list = [float(t['p95_latency_ms']) for t in trials if t['p95_latency_ms'] != 'N/A']
        p99_list = [float(t['p99_latency_ms']) for t in trials if t['p99_latency_ms'] != 'N/A']
        avg_lat_list = [float(t['avg_latency_ms']) for t in trials if t['avg_latency_ms'] != 'N/A']
        s2xx_list = [int(t['success_2xx']) for t in trials]
        s5xx_list = [int(t['http_5xx']) for t in trials]

        attempted_list = [int(t.get('attempted_requests', t.get('completed_http_responses', 0))) for t in trials]
        trans_err_list = [int(t.get('transport_errors', 0)) for t in trials]
        timeout_list = [int(t.get('timeouts', 0)) for t in trials]
        backoff_list = [float(t.get('total_backoff_sec', 0.0)) for t in trials]

        shed_rates = []
        for t in trials:
            s2 = int(t['success_2xx'])
            s5 = int(t['http_5xx'])
            tot = s2 + s5
            shed_rates.append((s5 / tot * 100.0) if tot > 0 else 0.0)

        results[c] = {
            'n': n,
            'rps_mean': statistics.mean(rps_list),
            'rps_stdev': statistics.stdev(rps_list),
            'p50_mean': statistics.mean(p50_list) if p50_list else None,
            'p50_stdev': statistics.stdev(p50_list) if len(p50_list) > 1 else 0.0,
            'p95_mean': statistics.mean(p95_list) if p95_list else None,
            'p95_stdev': statistics.stdev(p95_list) if len(p95_list) > 1 else 0.0,
            'p99_mean': statistics.mean(p99_list) if p99_list else None,
            'p99_stdev': statistics.stdev(p99_list) if len(p99_list) > 1 else 0.0,
            'avg_lat_mean': statistics.mean(avg_lat_list) if avg_lat_list else None,
            'avg_lat_stdev': statistics.stdev(avg_lat_list) if len(avg_lat_list) > 1 else 0.0,
            'att_mean': statistics.mean(attempted_list),
            'att_stdev': statistics.stdev(attempted_list),
            's2xx_mean': statistics.mean(s2xx_list),
            's2xx_stdev': statistics.stdev(s2xx_list),
            's5xx_mean': statistics.mean(s5xx_list),
            's5xx_stdev': statistics.stdev(s5xx_list),
            'shed_rate_mean': statistics.mean(shed_rates),
            'shed_rate_stdev': statistics.stdev(shed_rates),
            'trans_err_mean': statistics.mean(trans_err_list),
            'timeout_mean': statistics.mean(timeout_list),
            'backoff_mean': statistics.mean(backoff_list),
            'backoff_stdev': statistics.stdev(backoff_list) if len(backoff_list) > 1 else 0.0,
            'raw_trials': trials
        }
    return results

def print_summary(label, stats):
    print(f"\n--- {label} (5 trials per concurrency) ---")
    for c, s in stats.items():
        p50_s = f"{s['p50_mean']:5.2f} +/- {s['p50_stdev']:4.2f} ms" if s['p50_mean'] is not None else "     N/A          "
        p95_s = f"{s['p95_mean']:6.2f} +/- {s['p95_stdev']:5.2f} ms" if s['p95_mean'] is not None else "      N/A          "
        p99_s = f"{s['p99_mean']:6.2f} +/- {s['p99_stdev']:5.2f} ms" if s['p99_mean'] is not None else "      N/A          "
        bo_s = f" | Backoff = {s['backoff_mean']:5.2f}s" if s['backoff_mean'] > 0 else ""
        print(f"C={c:2d}: 2xx RPS = {s['rps_mean']:9.2f} +/- {s['rps_stdev']:6.2f} | "
              f"Attempts = {s['att_mean']:8.1f} | "
              f"p50 = {p50_s} | "
              f"p95 = {p95_s} | "
              f"p99 = {p99_s} | "
              f"5xx = {s['s5xx_mean']:9.2f} +/- {s['s5xx_stdev']:8.2f} | "
              f"Shed% = {s['shed_rate_mean']:5.2f}% +/- {s['shed_rate_stdev']:4.2f}%"
              f"{bo_s}")

def print_markdown_table(baseline_stats, qlaac_stats):
    print("\n" + "=" * 90)
    print("DETAILED PAIRED COMPARISON TABLE")
    print("=" * 90)
    print("| Concurrency ($C$) | Configuration | Attempts (Mean $\\pm$ Std) | 2xx RPS (Mean $\\pm$ Std) | $\\Delta$ RPS (%) | $p50$ (ms) | $p95$ (ms) | $p99$ (ms) | $\\Delta p99$ (%) | 5xx Shed | Backoff (s) | Classification |")
    print("| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |")
    for c in sorted(baseline_stats.keys()):
        b = baseline_stats[c]
        q = qlaac_stats[c]
        p50_b = f"{b['p50_mean']:.2f}" if b['p50_mean'] is not None else "N/A"
        p95_b = f"{b['p95_mean']:.2f}" if b['p95_mean'] is not None else "N/A"
        p99_b = f"{b['p99_mean']:.2f}" if b['p99_mean'] is not None else "N/A"

        p50_q = f"{q['p50_mean']:.2f}" if q['p50_mean'] is not None else "N/A"
        p95_q = f"{q['p95_mean']:.2f}" if q['p95_mean'] is not None else "N/A"
        p99_q = f"{q['p99_mean']:.2f}" if q['p99_mean'] is not None else "N/A"

        delta_rps = ((q['rps_mean'] - b['rps_mean']) / b['rps_mean'] * 100.0) if b['rps_mean'] > 0 else 0.0
        delta_p99 = None
        if b['p99_mean'] is not None and q['p99_mean'] is not None and b['p99_mean'] > 0:
            delta_p99 = ((q['p99_mean'] - b['p99_mean']) / b['p99_mean'] * 100.0)
        delta_p99_str = f"{delta_p99:+.1f}%" if delta_p99 is not None else "N/A"

        # Classification
        # Improved: defensible reduction in tail latency without excessive loss of useful throughput (> -15%)
        # Trade-off: tail latency improves but throughput or rejection cost is material
        # Unsuccessful: throughput collapses without meaningful latency benefit
        if delta_rps >= -15.0 and delta_p99 is not None and delta_p99 < 0:
            classification = "**Improved**"
        elif delta_p99 is not None and delta_p99 < -20.0 and delta_rps < -15.0:
            classification = "**Trade-off**"
        elif abs(delta_rps) <= 15.0 and abs(delta_p99 if delta_p99 else 0) <= 15.0:
            classification = "Equivalent"
        else:
            classification = "**Unsuccessful**"

        bo_b = f"{b['backoff_mean']:.2f}" if b['backoff_mean'] > 0 else "0.00"
        bo_q = f"{q['backoff_mean']:.2f}" if q['backoff_mean'] > 0 else "0.00"

        print(f"| **{c}** | Baseline | {b['att_mean']:,.1f} $\\pm$ {b['att_stdev']:.1f} | {b['rps_mean']:,.2f} $\\pm$ {b['rps_stdev']:.2f} | — | {p50_b} | {p95_b} | {p99_b} | — | {b['s5xx_mean']:,.1f} | {bo_b} | Baseline |")
        print(f"| **{c}** | QLAAC | {q['att_mean']:,.1f} $\\pm$ {q['att_stdev']:.1f} | {q['rps_mean']:,.2f} $\\pm$ {q['rps_stdev']:.2f} | {delta_rps:+.1f}% | {p50_q} | {p95_q} | {p99_q} | {delta_p99_str} | {q['s5xx_mean']:,.1f} | {bo_q} | {classification} |")

def main():
    import argparse
    parser = argparse.ArgumentParser(description="Compute benchmark statistics directly from raw CSV data.")
    parser.add_argument("--baseline", type=str, default=str(BASELINE_CSV), help="Path to baseline CSV")
    parser.add_argument("--qlaac", type=str, default=str(QLAAC_CSV), help="Path to QLAAC CSV")
    parser.add_argument("--v4-standard", action="store_true", help="Use v4 paired standard mode raw CSV files")
    parser.add_argument("--v4-retryaware", action="store_true", help="Use v4 paired retry-aware mode raw CSV files")
    parser.add_argument("--v3-standard", action="store_true", help="Use v3 standard mode raw CSV files")
    parser.add_argument("--v3-retryaware", action="store_true", help="Use v3 retry-aware mode raw CSV files")
    parser.add_argument("--v2", action="store_true", help="Use second-pass (v2) queueaware raw CSV files")
    parser.add_argument("--revised", action="store_true", help="Use first-revision queueaware raw CSV files")
    args = parser.parse_args()

    base_path = Path(args.baseline)
    qlaac_path = Path(args.qlaac)

    if args.v4_standard:
        base_path = BASE_DIR / "results" / "raw" / "experiment_baseline_standard_v4_paired_20261019.csv"
        qlaac_path = BASE_DIR / "results" / "raw" / "experiment_qlaac_standard_v4_paired_20261019.csv"
    elif args.v4_retryaware:
        base_path = BASE_DIR / "results" / "raw" / "experiment_baseline_retry-aware_v4_paired_20261019.csv"
        qlaac_path = BASE_DIR / "results" / "raw" / "experiment_qlaac_retry-aware_v4_paired_20261019.csv"
    elif args.v3_standard:
        base_path = BASE_DIR / "results" / "raw" / "experiment_baseline_standard_v3_20261019.csv"
        qlaac_path = BASE_DIR / "results" / "raw" / "experiment_qlaac_standard_v3_20261019.csv"
    elif args.v3_retryaware:
        base_path = BASE_DIR / "results" / "raw" / "experiment_baseline_retry-aware_v3_20261019.csv"
        qlaac_path = BASE_DIR / "results" / "raw" / "experiment_qlaac_retry-aware_v3_20261019.csv"
    elif args.v2:
        base_path = BASE_DIR / "results" / "raw" / "experiment_baseline_queueaware_v2_20261019.csv"
        qlaac_path = BASE_DIR / "results" / "raw" / "experiment_qlaac_queueaware_v2_20261019.csv"
    elif args.revised:
        base_path = BASE_DIR / "results" / "raw" / "experiment_baseline_queueaware_20261019.csv"
        qlaac_path = BASE_DIR / "results" / "raw" / "experiment_qlaac_queueaware_20261019.csv"

    print("=" * 80)
    print("COMPUTING EMPIRICAL BENCHMARK STATISTICS DIRECTLY FROM RAW CSV DATA")
    print(f" Baseline CSV: {base_path}")
    print(f" QLAAC CSV:    {qlaac_path}")
    print("=" * 80)

    baseline_stats = compute_metrics(base_path)
    qlaac_stats = compute_metrics(qlaac_path)

    print_summary("BASELINE RESULTS", baseline_stats)
    print_summary("QLAAC RESULTS", qlaac_stats)
    print_markdown_table(baseline_stats, qlaac_stats)

if __name__ == '__main__':
    main()
