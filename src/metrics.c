/**
 * @file metrics.c
 * @brief Thread-safe runtime metrics collection, latency histograms, and JSON telemetry export.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Intended Responsibilities:
 *  1. Thread-Safe Telemetry Collection:
 *     - Protect cumulative counters with POSIX mutex locks across all worker threads.
 *     - Track total connections accepted, rejected, and completed requests.
 *     - Track HTTP status code distributions (2xx, 3xx, 4xx, 5xx) and I/O byte volumes.
 *  2. Latency Profiling:
 *     - Record per-request response time in milliseconds into a circular reservoir.
 *     - Calculate rolling median (p50), 95th percentile (p95), and 99th percentile (p99).
 *  3. Dynamic Snapshotting:
 *     - Produce point-in-time telemetry snapshots (MetricsSnapshot) without holding locks across I/O.
 *  4. Observability Export:
 *     - Serialize snapshot into a clean JSON string for the '/metrics' HTTP endpoint and dashboard.
 *
 * Implementation Status:
 *  - Architecture specified in include/metrics.h.
 *  - Telemetry recording logic scheduled for development in Phase 4.
 */

#include "metrics.h"
#include <stdio.h>
#include <string.h>

void metrics_init(MetricsCollector *collector) {
    if (!collector) return;
    memset(collector, 0, sizeof(MetricsCollector));
    collector->server_start_time = time(NULL);
    pthread_mutex_init(&collector->lock, NULL);
}

void metrics_record_connection_accepted(MetricsCollector *collector) {
    if (!collector) return;
    pthread_mutex_lock(&collector->lock);
    collector->total_connections_accepted++;
    pthread_mutex_unlock(&collector->lock);
}

void metrics_record_connection_rejected(MetricsCollector *collector) {
    if (!collector) return;
    pthread_mutex_lock(&collector->lock);
    collector->total_connections_rejected++;
    pthread_mutex_unlock(&collector->lock);
}

void metrics_destroy(MetricsCollector *collector) {
    if (!collector) return;
    pthread_mutex_destroy(&collector->lock);
}
