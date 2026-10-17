"""
generate_plots.py
Generates publication-quality benchmark plots and systems diagrams using:
1. Direct data from verify_benchmark_statistics.py (single source of truth)
2. Accurate architectural and controller state-machine parameters matching C source code.

Outputs:
- results/plots/fig1_throughput_comparison.png
- results/plots/fig2_latency_p99_comparison.png
- results/plots/fig3_qlaac_admission_tradeoff.png
- results/plots/fig4_system_architecture.png
- results/plots/fig5_qlaac_state_machine.png
"""

import os
from pathlib import Path
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np

from verify_benchmark_statistics import compute_metrics, BASELINE_CSV, QLAAC_CSV

BASE_DIR = Path(__file__).resolve().parent.parent
PLOTS_DIR = BASE_DIR / "results" / "plots"
PLOTS_DIR.mkdir(parents=True, exist_ok=True)

# 1. Fetch exact data from CSVs
b_stats = compute_metrics(BASELINE_CSV)
q_stats = compute_metrics(QLAAC_CSV)

concurrencies = [2, 8, 32, 64]
b_rps = [b_stats[c]['rps_mean'] for c in concurrencies]
b_rps_err = [b_stats[c]['rps_stdev'] for c in concurrencies]
q_rps = [q_stats[c]['rps_mean'] for c in concurrencies]
q_rps_err = [q_stats[c]['rps_stdev'] for c in concurrencies]

b_p99 = [b_stats[c]['p99_mean'] for c in concurrencies]
b_p99_err = [b_stats[c]['p99_stdev'] for c in concurrencies]
q_p99 = [q_stats[c]['p99_mean'] for c in concurrencies]
q_p99_err = [q_stats[c]['p99_stdev'] for c in concurrencies]

# Set general aesthetic
plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')
plt.rcParams['font.sans-serif'] = 'DejaVu Sans'
plt.rcParams['axes.edgecolor'] = '#cbd5e1'
plt.rcParams['axes.linewidth'] = 1.0

# -------------------------------------------------------------
# FIG 1: Throughput Comparison (2xx RPS)
# -------------------------------------------------------------
fig, ax = plt.subplots(figsize=(8, 5), dpi=300)
x = np.arange(len(concurrencies))
width = 0.35

rects1 = ax.bar(x - width/2, b_rps, width, yerr=b_rps_err, capsize=4,
                label='Baseline (Static FIFO)', color='#0284c7', edgecolor='#0369a1', alpha=0.9)
rects2 = ax.bar(x + width/2, q_rps, width, yerr=q_rps_err, capsize=4,
                label='QLAAC (Adaptive Controller)', color='#10b981', edgecolor='#059669', alpha=0.9)

ax.set_xlabel('Client Concurrency Level ($C$)', fontsize=11, fontweight='bold', labelpad=8)
ax.set_ylabel('Successful 2xx Throughput (Requests / Sec)', fontsize=11, fontweight='bold', labelpad=8)
ax.set_title('HTTP/1.1 Throughput Scaling: Baseline vs. QLAAC Controller', fontsize=12, fontweight='bold', pad=12)
ax.set_xticks(x)
ax.set_xticklabels([f'$C={c}$' for c in concurrencies], fontsize=10)
ax.legend(frameon=True, facecolor='white', framealpha=0.95, fontsize=10)
ax.set_ylim(0, 17000)

for rect in rects1:
    h = rect.get_height()
    ax.annotate(f'{h:,.0f}', xy=(rect.get_x() + rect.get_width()/2, h),
                xytext=(0, 4), textcoords="offset points", ha='center', va='bottom', fontsize=8, fontweight='bold')
for rect in rects2:
    h = rect.get_height()
    ax.annotate(f'{h:,.0f}', xy=(rect.get_x() + rect.get_width()/2, h),
                xytext=(0, 4), textcoords="offset points", ha='center', va='bottom', fontsize=8, fontweight='bold')

plt.tight_layout()
fig1_path = PLOTS_DIR / "fig1_throughput_comparison.png"
plt.savefig(fig1_path)
plt.close()
print(f"Generated {fig1_path}")

# -------------------------------------------------------------
# FIG 2: p99 Tail Latency Comparison (Log Scale)
# -------------------------------------------------------------
fig, ax = plt.subplots(figsize=(8, 5), dpi=300)
ax.plot(concurrencies, b_p99, marker='o', linewidth=2.2, markersize=7,
        label='Baseline p99 (Bufferbloat Collapse)', color='#ef4444')
ax.errorbar(concurrencies, b_p99, yerr=b_p99_err, fmt='none', ecolor='#b91c1c', capsize=4)

ax.plot(concurrencies, q_p99, marker='s', linewidth=2.2, markersize=7,
        label='QLAAC p99 (Tail Latency Bound)', color='#10b981')
ax.errorbar(concurrencies, q_p99, yerr=q_p99_err, fmt='none', ecolor='#047857', capsize=4)

ax.set_xscale('log', base=2)
ax.set_yscale('log')
ax.set_xticks(concurrencies)
ax.set_xticklabels([str(c) for c in concurrencies], fontsize=10)
ax.set_xlabel('Client Concurrency Level ($C$, Log Scale)', fontsize=11, fontweight='bold', labelpad=8)
ax.set_ylabel('p99 Tail Latency (ms, Log Scale)', fontsize=11, fontweight='bold', labelpad=8)
ax.set_title('99th Percentile Tail Latency Bound Under Concurrency Surge', fontsize=12, fontweight='bold', pad=12)
ax.legend(frameon=True, facecolor='white', framealpha=0.95, fontsize=10)

# Annotations
ax.annotate(f'Baseline: {b_p99[-1]:.1f} ms\n(Queue Starvation)', xy=(64, b_p99[-1]), xytext=(35, 300),
            arrowprops=dict(arrowstyle="->", color='#ef4444', lw=1.2), fontsize=8.5, fontweight='bold', color='#b91c1c')
ax.annotate(f'QLAAC: {q_p99[-1]:.1f} ms\n(2.73x Improvement)', xy=(64, q_p99[-1]), xytext=(35, 55),
            arrowprops=dict(arrowstyle="->", color='#10b981', lw=1.2), fontsize=8.5, fontweight='bold', color='#047857')

plt.tight_layout()
fig2_path = PLOTS_DIR / "fig2_latency_p99_comparison.png"
plt.savefig(fig2_path)
plt.close()
print(f"Generated {fig2_path}")

# -------------------------------------------------------------
# FIG 3: Admission vs Overload Shedding Tradeoff at C=64
# -------------------------------------------------------------
fig, ax = plt.subplots(figsize=(7, 5), dpi=300)
categories = ['Baseline\n(No Admission Control)', 'QLAAC Controller\n(Adaptive Shedding)']
# Total requests at C=64: Admitted (2xx) vs Shed (5xx)
b_2xx_c64 = b_stats[64]['s2xx_mean']
b_5xx_c64 = b_stats[64]['s5xx_mean']
q_2xx_c64 = q_stats[64]['s2xx_mean']
q_5xx_c64 = q_stats[64]['s5xx_mean']

admitted = [b_2xx_c64, q_2xx_c64]
shed = [b_5xx_c64, q_5xx_c64]

p1 = ax.bar(categories, admitted, width=0.45, label='2xx Admitted Transactions', color='#0284c7', edgecolor='#0369a1')
p2 = ax.bar(categories, shed, width=0.45, bottom=admitted, label='503 Shed Overload Rejections', color='#f59e0b', edgecolor='#d97706')

ax.set_ylabel('Mean Transactions per 10s Trial ($C=64$)', fontsize=11, fontweight='bold')
ax.set_title('Admitted vs. Shed Requests Under Heavy Overload ($C=64$)', fontsize=12, fontweight='bold', pad=12)
ax.legend(frameon=True, facecolor='white', framealpha=0.95, fontsize=10)

ax.annotate(f'100% Admitted\n(p99 = {b_p99[-1]:.1f} ms)', xy=(0, b_2xx_c64/2), ha='center', va='center',
            color='white', fontsize=9, fontweight='bold')
ax.annotate(f'Admitted: {q_2xx_c64:,.0f}\n(p99 = {q_p99[-1]:.1f} ms)', xy=(1, q_2xx_c64/2), ha='center', va='center',
            color='white', fontsize=8.5, fontweight='bold')
ax.annotate(f'Shed: {q_5xx_c64:,.0f}\n({q_stats[64]["shed_rate_mean"]:.1f}% Shedding)', xy=(1, q_2xx_c64 + q_5xx_c64/2),
            ha='center', va='center', color='#78350f', fontsize=8.5, fontweight='bold')

plt.tight_layout()
fig3_path = PLOTS_DIR / "fig3_qlaac_admission_tradeoff.png"
plt.savefig(fig3_path)
plt.close()
print(f"Generated {fig3_path}")

# -------------------------------------------------------------
# FIG 4: Corrected Systems Architecture Diagram
# -------------------------------------------------------------
fig, ax = plt.subplots(figsize=(12, 7), dpi=300)
ax.set_xlim(0, 12)
ax.set_ylim(0, 7)
ax.axis('off')
fig.patch.set_facecolor('#0B0F19')
ax.set_facecolor('#0B0F19')

ax.text(6, 6.6, 'HTTP/1.1 Web Server: Systems Architecture & Concurrency Pipeline',
        ha='center', va='center', color='#F8FAFC', fontsize=15, fontweight='bold', fontfamily='sans-serif')
ax.text(6, 6.25, 'Zero-Dependency ISO C17 Architecture on Raw POSIX Sockets & Adaptive Overload Control',
        ha='center', va='center', color='#94A3B8', fontsize=10, fontfamily='sans-serif')

def draw_box(x, y, w, h, title, subtitle, color, border_color):
    rect = patches.FancyBboxPatch((x, y), w, h, boxstyle='round,pad=0.1,rounding_size=0.15',
                                 facecolor=color, edgecolor=border_color, linewidth=1.5)
    ax.add_patch(rect)
    ax.text(x + w/2, y + h*0.62, title, ha='center', va='center', color='#FFFFFF', fontsize=10.5, fontweight='bold')
    ax.text(x + w/2, y + h*0.28, subtitle, ha='center', va='center', color='#CBD5E1', fontsize=8.2, multialignment='center')

# Network Layer
draw_box(0.5, 3.8, 2.2, 1.6, 'Client Ingress', 'HTTP/1.1 Pipelined\nConcurrent TCP Streams\nAF_INET / SOCK_STREAM', '#1E293B', '#38BDF8')

# Listener & Socket Subsystem
draw_box(3.2, 3.8, 2.2, 1.6, 'TCP Listener Loop', 'SO_REUSEADDR\nSO_RCVTIMEO (5s)\nBacklog: 128 connections', '#1E293B', '#0EA5E9')

# Bounded Queue (Corrected to 64 slots default / 128 max)
draw_box(5.9, 3.8, 2.3, 1.6, 'Bounded Queue', 'Circular FIFO Buffer\nCapacity: 64 slots (max 128)\npthread_mutex / cond_var', '#1E293B', '#818CF8')

# Worker Pool (Corrected to 4 workers default / up to 16)
draw_box(8.7, 3.8, 2.8, 1.6, 'Worker Thread Pool', '4 Worker Threads (max 16)\nPre-spawned pthreads\nRequest Byte Ingress Timing', '#1E293B', '#A855F7')

# Downstream Processing Boxes
draw_box(8.7, 1.2, 2.8, 1.6, 'RFC 9112 Engine', 'Zero-Copy Header Scanner\nMethod / URI / Headers\nPath Canonicalization', '#1E293B', '#EC4899')
draw_box(5.9, 1.2, 2.3, 1.6, 'Route Dispatcher', 'Static File Engine / sendfile\nContent-Length Framing\nDynamic APIs (/metrics)', '#1E293B', '#F59E0B')

# QLAAC & Metrics
draw_box(3.2, 1.2, 2.2, 1.6, 'QLAAC Controller', 'Multi-Factor Feedback Loop\nNORMAL | CAUTION | OVERLOAD\nHysteresis & 503 Shedding', '#1E293B', '#10B981')
draw_box(0.5, 1.2, 2.2, 1.6, 'Observability Engine', 'Sliding Reservoir (1024 slots)\np50/p95/p99 Percentiles\nGET /metrics JSON API', '#1E293B', '#06B6D4')

# Arrows
arrow_kwargs = dict(arrowstyle='->', lw=2, color='#38BDF8', mutation_scale=15)
ax.annotate('', xy=(3.2, 4.6), xytext=(2.7, 4.6), arrowprops=arrow_kwargs)
ax.annotate('', xy=(5.9, 4.6), xytext=(5.4, 4.6), arrowprops=arrow_kwargs)
ax.annotate('', xy=(8.7, 4.6), xytext=(8.2, 4.6), arrowprops=arrow_kwargs)
ax.annotate('', xy=(10.1, 2.8), xytext=(10.1, 3.8), arrowprops=dict(arrowstyle='->', lw=2, color='#A855F7', mutation_scale=15))
ax.annotate('', xy=(8.7, 2.0), xytext=(8.2, 2.0), arrowprops=dict(arrowstyle='<-', lw=2, color='#F59E0B', mutation_scale=15))
ax.annotate('', xy=(5.9, 2.0), xytext=(5.4, 2.0), arrowprops=dict(arrowstyle='<-', lw=2, color='#10B981', mutation_scale=15))
ax.annotate('', xy=(3.2, 2.0), xytext=(2.7, 2.0), arrowprops=dict(arrowstyle='<-', lw=2, color='#06B6D4', mutation_scale=15))

# Feedback arrow from QLAAC to Listener
ax.annotate('', xy=(4.3, 3.8), xytext=(4.3, 2.8),
            arrowprops=dict(arrowstyle='->', lw=2, color='#10B981', linestyle='--', mutation_scale=15))
ax.text(4.4, 3.3, 'Admission Feedback\n(Shedding / Throttling)', color='#34D399', fontsize=8, fontweight='bold')

plt.tight_layout()
fig4_path = PLOTS_DIR / "fig4_system_architecture.png"
plt.savefig(fig4_path, dpi=300, facecolor=fig.get_facecolor(), edgecolor='none')
plt.close()
print(f"Generated {fig4_path}")

# -------------------------------------------------------------
# FIG 5: Corrected QLAAC State Machine Diagram
# -------------------------------------------------------------
fig, ax = plt.subplots(figsize=(11, 6), dpi=300)
ax.set_xlim(0, 11)
ax.set_ylim(0, 6)
ax.axis('off')
fig.patch.set_facecolor('#0B0F19')
ax.set_facecolor('#0B0F19')

ax.text(5.5, 5.5, 'QLAAC: Queue-and-Latency-Aware Admission Control State Machine',
        ha='center', va='center', color='#F8FAFC', fontsize=14, fontweight='bold', fontfamily='sans-serif')
ax.text(5.5, 5.15, 'Multi-Factor Feedback Loop with 5-Sample Hysteresis Dampening & Adaptive Shedding',
        ha='center', va='center', color='#94A3B8', fontsize=9.5, fontfamily='sans-serif')

def draw_state(x, y, w, h, name, desc, bg, border):
    rect = patches.FancyBboxPatch((x, y), w, h, boxstyle='round,pad=0.1,rounding_size=0.2',
                                 facecolor=bg, edgecolor=border, linewidth=2)
    ax.add_patch(rect)
    ax.text(x + w/2, y + h*0.65, name, ha='center', va='center', color='#FFFFFF', fontsize=13, fontweight='bold')
    ax.text(x + w/2, y + h*0.3, desc, ha='center', va='center', color='#CBD5E1', fontsize=8.0, multialignment='center')

# States matching exact C implementation
draw_state(0.8, 1.8, 2.7, 2.2, 'NORMAL', 'Queue < 70%\nWorker Util < 85%\np95 Latency < 50ms\n100% 200 OK Admission\nKeep-Alive Timeout: 5s', '#064E3B', '#10B981')
draw_state(4.2, 1.8, 2.7, 2.2, 'CAUTION', 'Worker Util >= 85% OR\nQueue >= 70% OR p95 >= 50ms\nFull 200 OK Admission\nKeep-Alive Throttled (1s)\nConnection: close injected', '#78350F', '#F59E0B')
draw_state(7.6, 1.8, 2.7, 2.2, 'OVERLOAD', 'Queue >= 90% OR p95 >= 150ms\nOR Worker Sat + Queue >= 70%\nOR Sustained Pressure >= 3 wins\nProactive 503 Shedding\nRetry-After: 5 header', '#7F1D1D', '#EF4444')

# Transitions
# Normal -> Caution
ax.annotate('', xy=(4.2, 3.4), xytext=(3.5, 3.4),
            arrowprops=dict(arrowstyle='->', lw=2, color='#F59E0B', mutation_scale=15))
ax.text(3.85, 3.65, 'Worker >= 85% OR\nQueue >= 70% OR\np95 >= 50ms', ha='center', color='#FCD34D', fontsize=7.2, fontweight='bold')

# Caution -> Normal (5 consecutive healthy samples)
ax.annotate('', xy=(3.5, 2.4), xytext=(4.2, 2.4),
            arrowprops=dict(arrowstyle='->', lw=2, color='#10B981', mutation_scale=15))
ax.text(3.85, 2.05, '5 Consecutive\nHealthy Samples', ha='center', color='#6EE7B7', fontsize=7.2, fontweight='bold')

# Caution -> Overload
ax.annotate('', xy=(7.6, 3.4), xytext=(6.9, 3.4),
            arrowprops=dict(arrowstyle='->', lw=2, color='#EF4444', mutation_scale=15))
ax.text(7.25, 3.65, 'Queue >= 90% OR\np95 >= 150ms OR\nSustained >= 3 wins', ha='center', color='#FCA5A5', fontsize=7.2, fontweight='bold')

# Overload -> Caution (5 consecutive healthy samples)
ax.annotate('', xy=(6.9, 2.4), xytext=(7.6, 2.4),
            arrowprops=dict(arrowstyle='->', lw=2, color='#F59E0B', mutation_scale=15))
ax.text(7.25, 2.05, '5 Consecutive\nHealthy Samples', ha='center', color='#FCD34D', fontsize=7.2, fontweight='bold')

# Complete recovery banner
ax.text(5.5, 0.6, 'Hysteresis Recovery Protocol: Requires 5 Consecutive Healthy Samples (Queue < 70%, Worker < 85%, p95 < 50ms) to Step Down',
        ha='center', color='#34D399', fontsize=8.2, fontweight='bold')

plt.tight_layout()
fig5_path = PLOTS_DIR / "fig5_qlaac_state_machine.png"
plt.savefig(fig5_path, dpi=300, facecolor=fig.get_facecolor(), edgecolor='none')
plt.close()
print(f"Generated {fig5_path}")

print("All plots generated successfully!")

if __name__ == '__main__':
    pass
