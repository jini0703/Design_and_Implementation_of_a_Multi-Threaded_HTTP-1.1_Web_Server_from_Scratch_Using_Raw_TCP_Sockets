#ifndef METRICS_H
#define METRICS_H

/**
 * @file metrics.h
 * @brief Thread-safe runtime metrics collection, latency histograms, and observability.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Module Responsibilities:
 *  - Provide thread-safe telemetry collection across all worker threads and listener.
 *  - Record accepted connections, active workers, queue depth, and completed requests.
 *  - Track response status code distribution (2xx, 3xx, 4xx, 5xx).
 *  - Maintain latency measurements using a sliding window / reservoir for p50, p95, and p99.
 *  - Measure rejection counts caused by queue overflow or adaptive admission control.
 *  - Export snapshot data for the JSON /metrics HTTP endpoint and QLAAC controller.
 */

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>

#define LATENCY_RESERVOIR_SIZE 1024

/**
 * @brief Point-in-time snapshot of server runtime metrics.
 */
typedef struct {
    uint64_t total_connections_accepted;
    uint64_t total_requests_completed;
    uint64_t total_connections_rejected;
    uint64_t status_2xx_count;
    uint64_t status_3xx_count;
    uint64_t status_4xx_count;
    uint64_t status_5xx_count;
    uint64_t bytes_received;
    uint64_t bytes_sent;
    size_t current_queue_depth;
    size_t active_workers;
    size_t total_workers;
    double requests_per_second;
    double median_latency_ms;
    double p95_latency_ms;
    double p99_latency_ms;
    double avg_latency_ms;
    time_t server_start_time;
    time_t snapshot_timestamp;
} MetricsSnapshot;

/**
 * @brief Thread-safe runtime metrics container.
 */
typedef struct {
    uint64_t total_connections_accepted;
    uint64_t total_requests_completed;
    uint64_t total_connections_rejected;
    uint64_t status_2xx_count;
    uint64_t status_3xx_count;
    uint64_t status_4xx_count;
    uint64_t status_5xx_count;
    uint64_t bytes_received;
    uint64_t bytes_sent;
    double latency_samples[LATENCY_RESERVOIR_SIZE];
    size_t latency_sample_count;
    size_t latency_index;
    time_t server_start_time;
    pthread_mutex_t lock;
} MetricsCollector;

/**
 * @brief Initialize the metrics collector.
 * @param collector Metrics collector to initialize.
 */
void metrics_init(MetricsCollector *collector);

/**
 * @brief Record an accepted client connection.
 * @param collector Target collector.
 */
void metrics_record_connection_accepted(MetricsCollector *collector);

/**
 * @brief Record a rejected connection due to capacity exhaustion or overload.
 * @param collector Target collector.
 */
void metrics_record_connection_rejected(MetricsCollector *collector);

/**
 * @brief Record completion of an HTTP request with its status and latency.
 * @param collector Target collector.
 * @param status_code HTTP status code (e.g. 200, 404, 503).
 * @param latency_ms Observed response duration in milliseconds.
 * @param bytes_read Number of request bytes consumed.
 * @param bytes_written Number of response bytes transmitted.
 */
void metrics_record_request(MetricsCollector *collector, int status_code,
                            double latency_ms, size_t bytes_read, size_t bytes_written);

/**
 * @brief Capture an atomic snapshot of current runtime metrics.
 * @param collector Source metrics collector.
 * @param current_queue_depth Instantaneous queue depth.
 * @param active_workers Instantaneous count of active worker threads.
 * @param total_workers Total configured worker threads.
 * @param snapshot Destination snapshot structure.
 */
void metrics_get_snapshot(MetricsCollector *collector, size_t current_queue_depth,
                          size_t active_workers, size_t total_workers,
                          MetricsSnapshot *snapshot);

/**
 * @brief Format a metrics snapshot into a JSON document for the /metrics route.
 * @param snapshot Source metrics snapshot.
 * @param buffer Output buffer.
 * @param buffer_size Capacity of buffer.
 * @return Number of bytes formatted, or -1 on buffer overflow.
 */
ssize_t metrics_format_json(const MetricsSnapshot *snapshot, char *buffer, size_t buffer_size);

/**
 * @brief Free resources and destroy metrics collector lock.
 * @param collector Collector to destroy.
 */
void metrics_destroy(MetricsCollector *collector);

#endif /* METRICS_H */
