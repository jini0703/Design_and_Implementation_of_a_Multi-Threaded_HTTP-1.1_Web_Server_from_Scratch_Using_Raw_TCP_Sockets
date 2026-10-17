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
            's2xx_mean': statistics.mean(s2xx_list),
            's2xx_stdev': statistics.stdev(s2xx_list),
            's5xx_mean': statistics.mean(s5xx_list),
            's5xx_stdev': statistics.stdev(s5xx_list),
            'shed_rate_mean': statistics.mean(shed_rates),
            'shed_rate_stdev': statistics.stdev(shed_rates),
            'raw_trials': trials
        }
    return results

def main():
    print("=" * 80)
    print("COMPUTING EMPIRICAL BENCHMARK STATISTICS DIRECTLY FROM RAW CSV DATA")
    print("=" * 80)

    baseline_stats = compute_metrics(BASELINE_CSV)
    qlaac_stats = compute_metrics(QLAAC_CSV)

    print("\n--- BASELINE RESULTS (5 trials per concurrency) ---")
    for c, s in baseline_stats.items():
        print(f"C={c:2d}: 2xx RPS = {s['rps_mean']:9.2f} +/- {s['rps_stdev']:6.2f} | "
              f"p50 = {s['p50_mean']:5.2f} +/- {s['p50_stdev']:4.2f} ms | "
              f"p95 = {s['p95_mean']:6.2f} +/- {s['p95_stdev']:5.2f} ms | "
              f"p99 = {s['p99_mean']:6.2f} +/- {s['p99_stdev']:5.2f} ms | "
              f"5xx = {s['s5xx_mean']:9.2f} +/- {s['s5xx_stdev']:8.2f} | "
              f"Shed% = {s['shed_rate_mean']:5.2f}% +/- {s['shed_rate_stdev']:4.2f}%")

    print("\n--- QLAAC RESULTS (5 trials per concurrency) ---")
    for c, s in qlaac_stats.items():
        print(f"C={c:2d}: 2xx RPS = {s['rps_mean']:9.2f} +/- {s['rps_stdev']:6.2f} | "
              f"p50 = {s['p50_mean']:5.2f} +/- {s['p50_stdev']:4.2f} ms | "
              f"p95 = {s['p95_mean']:6.2f} +/- {s['p95_stdev']:5.2f} ms | "
              f"p99 = {s['p99_mean']:6.2f} +/- {s['p99_stdev']:5.2f} ms | "
              f"5xx = {s['s5xx_mean']:9.2f} +/- {s['s5xx_stdev']:8.2f} | "
              f"Shed% = {s['shed_rate_mean']:5.2f}% +/- {s['shed_rate_stdev']:4.2f}%")

    print("\n" + "=" * 80)
    print("MARKDOWN TABLE FORMAT")
    print("=" * 80)
    print("| Concurrency ($C$) | Configuration | 2xx RPS (Mean $\\pm$ Std) | $p50$ Latency (ms) | $p95$ Latency (ms) | $p99$ Latency (ms) | 5xx Rejections (Mean $\\pm$ Std) | Shedding Rate (%) |")
    print("| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |")
    for c in [2, 8, 32, 64]:
        b = baseline_stats[c]
        q = qlaac_stats[c]
        print(f"| **{c}** | Baseline | {b['rps_mean']:,.2f} $\\pm$ {b['rps_stdev']:.2f} | {b['p50_mean']:.2f} $\\pm$ {b['p50_stdev']:.2f} | {b['p95_mean']:.2f} $\\pm$ {b['p95_stdev']:.2f} | {b['p99_mean']:.2f} $\\pm$ {b['p99_stdev']:.2f} | {b['s5xx_mean']:,.2f} $\\pm$ {b['s5xx_stdev']:.2f} | {b['shed_rate_mean']:.2f}% $\\pm$ {b['shed_rate_stdev']:.2f}% |")
        print(f"| **{c}** | QLAAC | {q['rps_mean']:,.2f} $\\pm$ {q['rps_stdev']:.2f} | {q['p50_mean']:.2f} $\\pm$ {q['p50_stdev']:.2f} | {q['p95_mean']:.2f} $\\pm$ {q['p95_stdev']:.2f} | {q['p99_mean']:.2f} $\\pm$ {q['p99_stdev']:.2f} | {q['s5xx_mean']:,.2f} $\\pm$ {q['s5xx_stdev']:.2f} | {q['shed_rate_mean']:.2f}% $\\pm$ {q['shed_rate_stdev']:.2f}% |")

if __name__ == '__main__':
    main()
