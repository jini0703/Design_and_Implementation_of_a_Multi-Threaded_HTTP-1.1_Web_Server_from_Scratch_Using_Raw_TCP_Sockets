#ifndef METRICS_H
#define METRICS_H

/**
 * @file metrics.h
 * @brief Thread-safe runtime metrics collection, latency histograms, and observability.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Metric Accounting Guidelines:
 *  - total_connections_accepted: Incremented on every valid client connection returned by accept(2).
 *  - total_connections_rejected: Incremented when a connection is rejected due to QLAAC overload or queue full.
 *  - total_overload_503_sent: Incremented ONLY when an HTTP 503 response is successfully written over the socket.
 *  - total_requests_completed: Incremented ONLY when a worker thread in the thread pool finishes an application request.
 *  - status_2xx, status_3xx, status_4xx, status_5xx: Categorical HTTP response counts. Overload 503s that were
 *    successfully transmitted increment status_5xx_count, but do NOT increment total_requests_completed.
 *  - bytes_sent: Total bytes successfully transferred across both application responses and overload rejections.
 */

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>

#define LATENCY_RESERVOIR_SIZE 1024
#define LATENCY_WINDOW_SECONDS 5.0

/**
 * @brief Timestamped latency observation for bounded-window percentile calculations.
 */
typedef struct {
    double latency_ms;
    uint64_t timestamp_ns;
} LatencySample;

/**
 * @brief Point-in-time snapshot of server runtime metrics.
 */
typedef struct {
    uint64_t total_connections_accepted;
    uint64_t total_requests_completed;
    uint64_t total_connections_rejected;
    uint64_t total_overload_503_sent;
    uint64_t status_2xx_count;
    uint64_t status_3xx_count;
    uint64_t status_4xx_count;
    uint64_t status_5xx_count;
    uint64_t bytes_received;
    uint64_t bytes_sent;
    size_t current_queue_depth;
    size_t active_workers;
    size_t total_workers;
    double requests_per_second;          /**< Rolling-window throughput over recent sampling interval */
    double lifetime_requests_per_second; /**< Cumulative lifetime throughput: total_requests / uptime */
    double median_latency_ms;           /**< p50 latency over recent bounded window */
    double p95_latency_ms;              /**< p95 latency over recent bounded window */
    double p99_latency_ms;              /**< p99 latency over recent bounded window */
    double avg_latency_ms;              /**< Mean latency over recent bounded window */
    size_t recent_latency_sample_count; /**< Number of valid latency samples within recent bounded window */
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
    uint64_t total_overload_503_sent;
    uint64_t status_2xx_count;
    uint64_t status_3xx_count;
    uint64_t status_4xx_count;
    uint64_t status_5xx_count;
    uint64_t bytes_received;
    uint64_t bytes_sent;
    LatencySample latency_samples[LATENCY_RESERVOIR_SIZE];
    size_t latency_sample_count;
    size_t latency_index;
    time_t server_start_time;
    uint64_t last_window_requests;
    uint64_t last_window_timestamp_ns;
    double rolling_rps;
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
 * @brief Record an overload HTTP response successfully transmitted on connection rejection.
 * Updates status_5xx_count, total_overload_503_sent, and bytes_sent.
 * Preserves separation by NOT incrementing total_requests_completed.
 * @param collector Target collector.
 * @param status_code HTTP status code (e.g. 503).
 * @param bytes_written Number of bytes successfully sent over socket.
 */
void metrics_record_overload_response(MetricsCollector *collector, int status_code, size_t bytes_written);

/**
 * @brief Record completion of an application HTTP request by a worker thread.
 * @param collector Target collector.
 * @param status_code HTTP status code (e.g. 200, 404, 500).
 * @param latency_ms Observed response duration in milliseconds.
 * @param bytes_read Number of request bytes consumed.
 * @param bytes_written Number of response bytes transmitted.
 */
void metrics_record_request(MetricsCollector *collector, int status_code,
                            double latency_ms, size_t bytes_read, size_t bytes_written);

/**
 * @brief Capture a thread-safe atomic snapshot of current runtime metrics.
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
