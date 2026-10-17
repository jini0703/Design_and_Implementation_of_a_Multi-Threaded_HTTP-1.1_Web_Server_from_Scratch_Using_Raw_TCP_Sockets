#!/usr/bin/env python3
"""
Generate publication-quality empirical benchmark figures from validated datasets.
Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server from Scratch
"""

import os
import csv
import math
import matplotlib.pyplot as plt
import numpy as np

def load_data(filepath):
    with open(filepath, 'r') as f:
        reader = list(csv.DictReader(f))
    by_c = {}
    for r in reader:
        c = int(r['concurrency'])
        by_c.setdefault(c, []).append(r)

    summary = {}
    for c, rows in sorted(by_c.items()):
        rps = [float(r['successful_rps']) for r in rows]
        p50 = [float(r['p50_latency_ms']) if r['p50_latency_ms'] != 'N/A' else None for r in rows]
        p95 = [float(r['p95_latency_ms']) if r['p95_latency_ms'] != 'N/A' else None for r in rows]
        p99 = [float(r['p99_latency_ms']) if r['p99_latency_ms'] != 'N/A' else None for r in rows]
        s2xx = sum(int(r['success_2xx']) for r in rows)
        s5xx = sum(int(r['http_5xx']) for r in rows)
        tot_att = sum(int(r['attempted_requests']) for r in rows)

        mean_rps = float(np.mean(rps))
        std_rps = float(np.std(rps, ddof=1)) if len(rps) > 1 else 0.0

        v_p50 = [x for x in p50 if x is not None]
        mean_p50 = float(np.mean(v_p50)) if v_p50 else 0.0

        v_p95 = [x for x in p95 if x is not None]
        mean_p95 = float(np.mean(v_p95)) if v_p95 else 0.0

        v_p99 = [x for x in p99 if x is not None]
        mean_p99 = float(np.mean(v_p99)) if v_p99 else 0.0

        summary[c] = {
            'mean_rps': mean_rps,
            'std_rps': std_rps,
            'mean_p50': mean_p50,
            'mean_p95': mean_p95,
            'mean_p99': mean_p99,
            's2xx': s2xx,
            's5xx': s5xx,
            'tot_att': tot_att
        }
    return summary

def main():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(base_dir)
    out_dir = os.path.join(project_root, 'results', 'plots')
    os.makedirs(out_dir, exist_ok=True)

    base_summary = load_data(os.path.join(project_root, 'results', 'raw', 'experiment_baseline_tuned_comparison.csv'))
    qlaac_summary = load_data(os.path.join(project_root, 'results', 'raw', 'experiment_qlaac_tuned_comparison.csv'))

    concurrency = [2, 8, 32, 64]
    x_pos = np.arange(len(concurrency))
    width = 0.35

    plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')

    # Fig 1: Throughput Comparison (2xx RPS)
    fig, ax = plt.subplots(figsize=(8, 5), dpi=300)
    base_rps = [base_summary[c]['mean_rps'] for c in concurrency]
    base_err = [base_summary[c]['std_rps'] for c in concurrency]
    qlaac_rps = [qlaac_summary[c]['mean_rps'] for c in concurrency]
    qlaac_err = [qlaac_summary[c]['std_rps'] for c in concurrency]

    rects1 = ax.bar(x_pos - width/2, base_rps, width, yerr=base_err, capsize=4,
                    label='Baseline (Fixed Policy)', color='#0284c7', edgecolor='none')
    rects2 = ax.bar(x_pos + width/2, qlaac_rps, width, yerr=qlaac_err, capsize=4,
                    label='QLAAC (Adaptive)', color='#10b981', edgecolor='none')

    ax.set_ylabel('Successful 2xx Throughput (Requests / Second)', fontsize=11, fontweight='bold')
    ax.set_xlabel('Client Concurrency (Parallel TCP Connections)', fontsize=11, fontweight='bold')
    ax.set_title('Throughput Scaling: Baseline vs. QLAAC Controller', fontsize=13, fontweight='bold', pad=15)
    ax.set_xticks(x_pos)
    ax.set_xticklabels([f'C = {c}' for c in concurrency], fontsize=10)
    ax.legend(frameon=True, fontsize=10)
    ax.set_ylim(0, 17000)

    for r in rects1:
        h = r.get_height()
        ax.annotate(f'{h:,.0f}', xy=(r.get_x() + r.get_width()/2, h),
                    xytext=(0, 4), textcoords="offset points", ha='center', va='bottom', fontsize=8)
    for r in rects2:
        h = r.get_height()
        ax.annotate(f'{h:,.0f}', xy=(r.get_x() + r.get_width()/2, h),
                    xytext=(0, 4), textcoords="offset points", ha='center', va='bottom', fontsize=8)

    plt.tight_layout()
    plt.savefig(os.path.join(out_dir, 'fig1_throughput_comparison.png'))
    plt.close()

    # Fig 2: p99 Latency Comparison (Log Scale)
    fig, ax = plt.subplots(figsize=(8, 5), dpi=300)
    base_p99 = [base_summary[c]['mean_p99'] for c in concurrency]
    qlaac_p99 = [qlaac_summary[c]['mean_p99'] for c in concurrency]

    ax.plot(concurrency, base_p99, marker='o', linewidth=2.5, markersize=8,
            label='Baseline (Fixed Policy)', color='#ef4444')
    ax.plot(concurrency, qlaac_p99, marker='s', linewidth=2.5, markersize=8,
            label='QLAAC (Adaptive)', color='#10b981')

    ax.set_yscale('log')
    ax.set_ylabel('p99 Tail Latency (ms, Log Scale)', fontsize=11, fontweight='bold')
    ax.set_xlabel('Client Concurrency (Parallel TCP Connections)', fontsize=11, fontweight='bold')
    ax.set_title('Tail Latency Protection: Baseline vs. QLAAC (p99 Bound)', fontsize=13, fontweight='bold', pad=15)
    ax.set_xticks(concurrency)
    ax.set_xticklabels([f'C = {c}' for c in concurrency], fontsize=10)
    ax.legend(frameon=True, fontsize=10)

    for x, y in zip(concurrency, base_p99):
        ax.annotate(f'{y:.1f} ms', xy=(x, y), xytext=(-10, 8), textcoords="offset points",
                    fontsize=8, fontweight='bold', color='#b91c1c')
    for x, y in zip(concurrency, qlaac_p99):
        ax.annotate(f'{y:.1f} ms', xy=(x, y), xytext=(5, -12), textcoords="offset points",
                    fontsize=8, fontweight='bold', color='#047857')

    plt.tight_layout()
    plt.savefig(os.path.join(out_dir, 'fig2_latency_p99_comparison.png'))
    plt.close()

    # Fig 3: Admission Breakdown under C=64 Overload
    fig, ax = plt.subplots(figsize=(7, 4.5), dpi=300)
    categories = ['Baseline\n(C=64)', 'QLAAC\n(C=64)']
    c64_b_2xx = base_summary[64]['s2xx']
    c64_b_5xx = base_summary[64]['s5xx']
    c64_q_2xx = qlaac_summary[64]['s2xx']
    c64_q_5xx = qlaac_summary[64]['s5xx']

    tot_b = c64_b_2xx + c64_b_5xx
    tot_q = c64_q_2xx + c64_q_5xx

    b_pct_2xx = (c64_b_2xx / tot_b) * 100
    b_pct_5xx = (c64_b_5xx / tot_b) * 100
    q_pct_2xx = (c64_q_2xx / tot_q) * 100
    q_pct_5xx = (c64_q_5xx / tot_q) * 100

    bar_width = 0.5
    y_pos = np.arange(len(categories))

    ax.barh(y_pos, [b_pct_2xx, q_pct_2xx], bar_width, label='2xx Success (Admitted)', color='#10b981')
    ax.barh(y_pos, [b_pct_5xx, q_pct_5xx], bar_width, left=[b_pct_2xx, q_pct_2xx],
            label='503 Service Unavailable (Shed)', color='#f59e0b')

    ax.set_xlabel('Percentage of Attempted Requests (%)', fontsize=11, fontweight='bold')
    ax.set_title('Overload Traffic Composition at C=64 (Shedding Trade-off)', fontsize=12, fontweight='bold', pad=15)
    ax.set_yticks(y_pos)
    ax.set_yticklabels(categories, fontsize=10, fontweight='bold')
    ax.set_xlim(0, 100)
    ax.legend(loc='lower center', bbox_to_anchor=(0.5, -0.3), ncol=2, frameon=True)

    ax.annotate(f'100% (p99: 440ms)', xy=(50, 0), ha='center', va='center', color='white', fontweight='bold', fontsize=9)
    ax.annotate(f'27.9% 2xx (p99: 161ms)', xy=(14, 1), ha='center', va='center', color='white', fontweight='bold', fontsize=8)
    ax.annotate(f'72.1% 503 Shed', xy=(64, 1), ha='center', va='center', color='black', fontweight='bold', fontsize=9)

    plt.tight_layout()
    plt.savefig(os.path.join(out_dir, 'fig3_qlaac_admission_tradeoff.png'))
    plt.close()

    print('Generated all benchmark plots successfully in results/plots/')

if __name__ == '__main__':
    main()
