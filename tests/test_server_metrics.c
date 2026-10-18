/**
 * @file test_server_metrics.c
 * @brief Unit test suite for server-side HTTP metrics accounting and overload distinction.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "metrics.h"

static void test_metrics_initialization(void) {
    MetricsCollector mc;
    metrics_init(&mc);

    MetricsSnapshot snap;
    metrics_get_snapshot(&mc, 0, 0, 8, &snap);

    assert(snap.total_connections_accepted == 0);
    assert(snap.total_connections_rejected == 0);
    assert(snap.total_requests_completed == 0);
    assert(snap.status_2xx_count == 0);
    assert(snap.status_3xx_count == 0);
    assert(snap.status_4xx_count == 0);
    assert(snap.status_5xx_count == 0);
    assert(snap.total_overload_503_sent == 0);
    assert(snap.bytes_sent == 0);
    assert(snap.bytes_received == 0);
    assert(snap.current_queue_depth == 0);
    assert(snap.active_workers == 0);
    assert(snap.total_workers == 8);

    metrics_destroy(&mc);
    printf("[PASS] test_metrics_initialization\n");
}

static void test_metrics_normal_request_accounting(void) {
    MetricsCollector mc;
    metrics_init(&mc);

    metrics_record_connection_accepted(&mc);

    /* Record 200 OK with 120 bytes read and 512 bytes written */
    metrics_record_request(&mc, 200, 15.0, 120, 512);

    /* Record 404 Not Found with 80 bytes read and 256 bytes written */
    metrics_record_request(&mc, 404, 5.0, 80, 256);

    MetricsSnapshot snap;
    metrics_get_snapshot(&mc, 2, 4, 8, &snap);

    assert(snap.total_connections_accepted == 1);
    assert(snap.total_connections_rejected == 0);
    assert(snap.total_requests_completed == 2);
    assert(snap.status_2xx_count == 1);
    assert(snap.status_4xx_count == 1);
    assert(snap.status_5xx_count == 0);
    assert(snap.total_overload_503_sent == 0);
    assert(snap.bytes_received == 200);
    assert(snap.bytes_sent == 768);
    assert(snap.current_queue_depth == 2);
    assert(snap.active_workers == 4);

    metrics_destroy(&mc);
    printf("[PASS] test_metrics_normal_request_accounting\n");
}

static void test_metrics_overload_response_accounting(void) {
    MetricsCollector mc;
    metrics_init(&mc);

    /* Simulate an incoming connection accepted */
    metrics_record_connection_accepted(&mc);

    /* Overload rejection occurs: connection rejected counter increments */
    metrics_record_connection_rejected(&mc);

    /* Overload HTTP 503 response successfully transmitted over socket (185 bytes) */
    metrics_record_overload_response(&mc, 503, 185);

    MetricsSnapshot snap;
    metrics_get_snapshot(&mc, 0, 0, 8, &snap);

    /* Invariant: total_connections_rejected is 1 */
    assert(snap.total_connections_rejected == 1);

    /* Invariant: status_5xx_count is 1 and total_overload_503_sent is 1 */
    assert(snap.status_5xx_count == 1);
    assert(snap.total_overload_503_sent == 1);
    assert(snap.bytes_sent == 185);

    /* CRITICAL INVARIANT: total_requests_completed MUST REMAIN 0!
     * A rejected TCP connection does not correspond to a fully processed application request. */
    assert(snap.total_requests_completed == 0);

    metrics_destroy(&mc);
    printf("[PASS] test_metrics_overload_response_accounting\n");
}

static void test_metrics_failed_overload_write(void) {
    MetricsCollector mc;
    metrics_init(&mc);

    metrics_record_connection_accepted(&mc);

    /* Overload rejection occurs */
    metrics_record_connection_rejected(&mc);

    /* Socket write failed before bytes could be sent; metrics_record_overload_response is NOT called */
    MetricsSnapshot snap;
    metrics_get_snapshot(&mc, 0, 0, 8, &snap);

    assert(snap.total_connections_rejected == 1);
    assert(snap.status_5xx_count == 0);
    assert(snap.total_overload_503_sent == 0);
    assert(snap.bytes_sent == 0);
    assert(snap.total_requests_completed == 0);

    metrics_destroy(&mc);
    printf("[PASS] test_metrics_failed_overload_write\n");
}

static void test_metrics_bounded_latency_window(void) {
    MetricsCollector mc;
    metrics_init(&mc);

    /* Record 3 requests with latencies 10ms, 20ms, 30ms */
    metrics_record_request(&mc, 200, 10.0, 100, 200);
    metrics_record_request(&mc, 200, 20.0, 100, 200);
    metrics_record_request(&mc, 200, 30.0, 100, 200);

    MetricsSnapshot snap;
    metrics_get_snapshot(&mc, 0, 0, 4, &snap);

    assert(snap.total_requests_completed == 3);
    assert(snap.recent_latency_sample_count == 3);
    assert(snap.avg_latency_ms == 20.0);
    assert(snap.median_latency_ms == 20.0);
    assert(snap.p95_latency_ms == 30.0);

    /* Now artificially expire the samples by shifting their timestamp back by 10 seconds */
    pthread_mutex_lock(&mc.lock);
    for (size_t i = 0; i < mc.latency_sample_count; i++) {
        if (mc.latency_samples[i].timestamp_ns > 10ULL * 1000000000ULL) {
            mc.latency_samples[i].timestamp_ns -= 10ULL * 1000000000ULL;
        } else {
            mc.latency_samples[i].timestamp_ns = 0;
        }
    }
    pthread_mutex_unlock(&mc.lock);

    /* A new snapshot after window expiry must report 0.0 for recent latency and 0 recent samples */
    metrics_get_snapshot(&mc, 0, 0, 4, &snap);
    assert(snap.recent_latency_sample_count == 0);
    assert(snap.avg_latency_ms == 0.0);
    assert(snap.median_latency_ms == 0.0);
    assert(snap.p95_latency_ms == 0.0);
    assert(snap.p99_latency_ms == 0.0);

    metrics_destroy(&mc);
    printf("[PASS] test_metrics_bounded_latency_window\n");
}

static void test_metrics_negative_latency_excluded(void) {
    MetricsCollector mc;
    metrics_init(&mc);

    /* Record request with negative latency (e.g. /metrics telemetry request where lat = -1.0) */
    metrics_record_request(&mc, 200, -1.0, 150, 400);

    MetricsSnapshot snap;
    metrics_get_snapshot(&mc, 0, 0, 4, &snap);

    assert(snap.total_requests_completed == 1);
    assert(snap.status_2xx_count == 1);
    assert(snap.recent_latency_sample_count == 0);
    assert(snap.avg_latency_ms == 0.0);
    assert(snap.median_latency_ms == 0.0);
    assert(snap.p95_latency_ms == 0.0);

    /* Now record an application request with valid positive latency (15.0 ms) */
    metrics_record_request(&mc, 200, 15.0, 120, 300);

    metrics_get_snapshot(&mc, 0, 0, 4, &snap);
    assert(snap.total_requests_completed == 2);
    assert(snap.status_2xx_count == 2);
    assert(snap.recent_latency_sample_count == 1);
    assert(snap.avg_latency_ms == 15.0);
    assert(snap.median_latency_ms == 15.0);
    assert(snap.p95_latency_ms == 15.0);

    metrics_destroy(&mc);
    printf("[PASS] test_metrics_negative_latency_excluded\n");
}

static void test_metrics_format_json_serialization(void) {
    MetricsCollector mc;
    metrics_init(&mc);

    metrics_record_request(&mc, 200, 25.0, 100, 200);

    MetricsSnapshot snap;
    metrics_get_snapshot(&mc, 1, 2, 4, &snap);

    char json_buf[4096];
    ssize_t written = metrics_format_json(&snap, json_buf, sizeof(json_buf));
    assert(written > 0);
    assert(strstr(json_buf, "\"recent_latency_sample_count\": 1") != NULL);
    assert(strstr(json_buf, "\"total_requests_completed\": 1") != NULL);
    assert(strstr(json_buf, "\"median_latency_ms\": 25.00") != NULL);

    metrics_destroy(&mc);
    printf("[PASS] test_metrics_format_json_serialization\n");
}

int main(void) {
    printf("--- Running Server Metrics Unit Tests ---\n");
    test_metrics_initialization();
    test_metrics_normal_request_accounting();
    test_metrics_overload_response_accounting();
    test_metrics_failed_overload_write();
    test_metrics_bounded_latency_window();
    test_metrics_negative_latency_excluded();
    test_metrics_format_json_serialization();
    printf("--- All Server Metrics Tests Passed ---\n");
    return EXIT_SUCCESS;
}
