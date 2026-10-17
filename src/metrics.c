/**
 * @file metrics.c
 * @brief Thread-safe runtime metrics collection, latency histograms, and JSON telemetry export.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include "metrics.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int compare_doubles(const void *a, const void *b) {
    double da = *(const double *)a;
    double db = *(const double *)b;
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

void metrics_init(MetricsCollector *collector) {
    if (!collector) return;
    memset(collector, 0, sizeof(MetricsCollector));
    collector->server_start_time = time(NULL);
    collector->last_window_timestamp_ns = monotonic_ns();
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

void metrics_record_overload_response(MetricsCollector *collector, int status_code, size_t bytes_written) {
    if (!collector) return;

    pthread_mutex_lock(&collector->lock);

    collector->bytes_sent += bytes_written;

    if (status_code >= 500 && status_code < 600) {
        collector->status_5xx_count++;
        if (status_code == 503) {
            collector->total_overload_503_sent++;
        }
    } else if (status_code >= 400 && status_code < 500) {
        collector->status_4xx_count++;
    }

    pthread_mutex_unlock(&collector->lock);
}

void metrics_record_request(MetricsCollector *collector, int status_code,
                            double latency_ms, size_t bytes_read, size_t bytes_written) {
    if (!collector) return;

    pthread_mutex_lock(&collector->lock);

    collector->total_requests_completed++;
    collector->bytes_received += bytes_read;
    collector->bytes_sent += bytes_written;

    if (status_code >= 200 && status_code < 300) {
        collector->status_2xx_count++;
    } else if (status_code >= 300 && status_code < 400) {
        collector->status_3xx_count++;
    } else if (status_code >= 400 && status_code < 500) {
        collector->status_4xx_count++;
    } else if (status_code >= 500 && status_code < 600) {
        collector->status_5xx_count++;
    }

    if (latency_ms >= 0.0) {
        collector->latency_samples[collector->latency_index].latency_ms = latency_ms;
        collector->latency_samples[collector->latency_index].timestamp_ns = monotonic_ns();
        collector->latency_index = (collector->latency_index + 1) % LATENCY_RESERVOIR_SIZE;
        if (collector->latency_sample_count < LATENCY_RESERVOIR_SIZE) {
            collector->latency_sample_count++;
        }
    }

    pthread_mutex_unlock(&collector->lock);
}

void metrics_get_snapshot(MetricsCollector *collector, size_t current_queue_depth,
                          size_t active_workers, size_t total_workers,
                          MetricsSnapshot *snapshot) {
    if (!collector || !snapshot) return;

    memset(snapshot, 0, sizeof(MetricsSnapshot));

    double temp_latencies[LATENCY_RESERVOIR_SIZE];
    size_t count = 0;
    time_t now = time(NULL);
    uint64_t now_ns = monotonic_ns();
    uint64_t window_ns = (uint64_t)(LATENCY_WINDOW_SECONDS * (double)NS_PER_SEC);

    pthread_mutex_lock(&collector->lock);

    snapshot->total_connections_accepted = collector->total_connections_accepted;
    snapshot->total_requests_completed = collector->total_requests_completed;
    snapshot->total_connections_rejected = collector->total_connections_rejected;
    snapshot->total_overload_503_sent = collector->total_overload_503_sent;
    snapshot->status_2xx_count = collector->status_2xx_count;
    snapshot->status_3xx_count = collector->status_3xx_count;
    snapshot->status_4xx_count = collector->status_4xx_count;
    snapshot->status_5xx_count = collector->status_5xx_count;
    snapshot->bytes_received = collector->bytes_received;
    snapshot->bytes_sent = collector->bytes_sent;
    snapshot->server_start_time = collector->server_start_time;
    snapshot->snapshot_timestamp = now;

    /* Compute rolling requests_per_second over recent sampling interval */
    uint64_t delta_reqs = collector->total_requests_completed - collector->last_window_requests;
    uint64_t delta_ns = now_ns - collector->last_window_timestamp_ns;
    if (delta_ns >= 500 * NS_PER_MS) {
        collector->rolling_rps = ((double)delta_reqs) / ((double)delta_ns / 1.0e9);
        collector->last_window_requests = collector->total_requests_completed;
        collector->last_window_timestamp_ns = now_ns;
    }
    snapshot->requests_per_second = collector->rolling_rps;

    /* Filter latency samples: only retain observations within the recent bounded window */
    size_t total_samples = collector->latency_sample_count;
    if (total_samples > LATENCY_RESERVOIR_SIZE) total_samples = LATENCY_RESERVOIR_SIZE;

    for (size_t i = 0; i < total_samples; i++) {
        uint64_t sample_ts = collector->latency_samples[i].timestamp_ns;
        if (now_ns >= sample_ts && (now_ns - sample_ts) <= window_ns) {
            temp_latencies[count++] = collector->latency_samples[i].latency_ms;
        }
    }

    pthread_mutex_unlock(&collector->lock);

    snapshot->current_queue_depth = current_queue_depth;
    snapshot->active_workers = active_workers;
    snapshot->total_workers = total_workers;

    /* Cumulative lifetime throughput */
    double uptime = difftime(now, snapshot->server_start_time);
    if (uptime > 0.0) {
        snapshot->lifetime_requests_per_second = (double)snapshot->total_requests_completed / uptime;
    } else {
        snapshot->lifetime_requests_per_second = (double)snapshot->total_requests_completed;
    }

    snapshot->recent_latency_sample_count = count;

    if (count > 0) {
        double sum = 0.0;
        for (size_t i = 0; i < count; i++) {
            sum += temp_latencies[i];
        }
        snapshot->avg_latency_ms = sum / (double)count;

        qsort(temp_latencies, count, sizeof(double), compare_doubles);

        size_t idx50 = (size_t)(0.50 * (count - 1) + 0.5);
        size_t idx95 = (size_t)(0.95 * (count - 1) + 0.5);
        size_t idx99 = (size_t)(0.99 * (count - 1) + 0.5);
        if (idx50 >= count) idx50 = count - 1;
        if (idx95 >= count) idx95 = count - 1;
        if (idx99 >= count) idx99 = count - 1;

        snapshot->median_latency_ms = temp_latencies[idx50];
        snapshot->p95_latency_ms = temp_latencies[idx95];
        snapshot->p99_latency_ms = temp_latencies[idx99];
    } else {
        snapshot->avg_latency_ms = 0.0;
        snapshot->median_latency_ms = 0.0;
        snapshot->p95_latency_ms = 0.0;
        snapshot->p99_latency_ms = 0.0;
    }
}

ssize_t metrics_format_json(const MetricsSnapshot *snapshot, char *buffer, size_t buffer_size) {
    if (!snapshot || !buffer || buffer_size == 0) return -1;

    int written = snprintf(buffer, buffer_size,
        "{\n"
        "  \"total_connections_accepted\": %llu,\n"
        "  \"total_requests_completed\": %llu,\n"
        "  \"total_connections_rejected\": %llu,\n"
        "  \"total_overload_503_sent\": %llu,\n"
        "  \"status_codes\": {\n"
        "    \"2xx\": %llu,\n"
        "    \"3xx\": %llu,\n"
        "    \"4xx\": %llu,\n"
        "    \"5xx\": %llu\n"
        "  },\n"
        "  \"bytes_received\": %llu,\n"
        "  \"bytes_sent\": %llu,\n"
        "  \"current_queue_depth\": %zu,\n"
        "  \"active_workers\": %zu,\n"
        "  \"total_workers\": %zu,\n"
        "  \"requests_per_second\": %.2f,\n"
        "  \"lifetime_requests_per_second\": %.2f,\n"
        "  \"median_latency_ms\": %.2f,\n"
        "  \"p95_latency_ms\": %.2f,\n"
        "  \"p99_latency_ms\": %.2f,\n"
        "  \"avg_latency_ms\": %.2f,\n"
        "  \"uptime_seconds\": %ld,\n"
        "  \"snapshot_timestamp\": %ld\n"
        "}\n",
        (unsigned long long)snapshot->total_connections_accepted,
        (unsigned long long)snapshot->total_requests_completed,
        (unsigned long long)snapshot->total_connections_rejected,
        (unsigned long long)snapshot->total_overload_503_sent,
        (unsigned long long)snapshot->status_2xx_count,
        (unsigned long long)snapshot->status_3xx_count,
        (unsigned long long)snapshot->status_4xx_count,
        (unsigned long long)snapshot->status_5xx_count,
        (unsigned long long)snapshot->bytes_received,
        (unsigned long long)snapshot->bytes_sent,
        snapshot->current_queue_depth,
        snapshot->active_workers,
        snapshot->total_workers,
        snapshot->requests_per_second,
        snapshot->lifetime_requests_per_second,
        snapshot->median_latency_ms,
        snapshot->p95_latency_ms,
        snapshot->p99_latency_ms,
        snapshot->avg_latency_ms,
        (long)(snapshot->snapshot_timestamp - snapshot->server_start_time),
        (long)snapshot->snapshot_timestamp
    );

    if (written < 0 || (size_t)written >= buffer_size) {
        return -1;
    }
    return (ssize_t)written;
}

void metrics_destroy(MetricsCollector *collector) {
    if (!collector) return;
    pthread_mutex_destroy(&collector->lock);
}
