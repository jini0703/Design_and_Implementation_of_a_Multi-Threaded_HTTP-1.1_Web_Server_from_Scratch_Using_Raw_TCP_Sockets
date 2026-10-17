/**
 * @file router.c
 * @brief HTTP request dispatching, URI routing, and endpoint handler mapping.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include "router.h"
#include "server.h"
#include "static_files.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int router_dispatch(ServerContext *ctx, const HttpRequest *req, HttpResponse *res) {
    if (!ctx || !req || !res) return -1;

    res->is_keep_alive = req->keep_alive;
    if (req->method == HTTP_METHOD_HEAD) {
        res->is_head_only = true;
    }

    /* Reject unsupported HTTP methods according to specification */
    if (req->method == HTTP_METHOD_UNKNOWN) {
        http_response_build_error(res, HTTP_STATUS_NOT_IMPLEMENTED,
                                  "HTTP method is not implemented on this server.", res->is_keep_alive);
        return 0;
    }

    /* 1. Liveness / Readiness Health Check Endpoint */
    if (strcmp(req->uri, "/health") == 0 || strcmp(req->uri, "/status") == 0) {
        if (req->method != HTTP_METHOD_GET && req->method != HTTP_METHOD_HEAD) {
            http_response_build_error(res, HTTP_STATUS_METHOD_NOT_ALLOWED,
                                      "Only GET and HEAD are permitted on /health.", res->is_keep_alive);
            return 0;
        }

        const char *health_json =
            "{\n"
            "  \"status\": \"healthy\",\n"
            "  \"server\": \"" SERVER_VERSION_STRING "\",\n"
            "  \"protocol\": \"HTTP/1.1\"\n"
            "}\n";
        http_response_set_status(res, HTTP_STATUS_OK);
        http_response_set_body_copy(res, "application/json; charset=utf-8", health_json, strlen(health_json));
        return 0;
    }

    /* 2. Runtime Metrics Observability Endpoint */
    const char *metrics_route = (ctx->config.metrics_route[0] != '\0') ? ctx->config.metrics_route : "/metrics";
    if (strcmp(req->uri, metrics_route) == 0 || strcmp(req->uri, "/metrics") == 0) {
        if (!ctx->config.metrics_enabled) {
            http_response_build_error(res, HTTP_STATUS_NOT_FOUND,
                                      "Metrics endpoint is disabled.", res->is_keep_alive);
            return 0;
        }
        if (req->method != HTTP_METHOD_GET && req->method != HTTP_METHOD_HEAD) {
            http_response_build_error(res, HTTP_STATUS_METHOD_NOT_ALLOWED,
                                      "Only GET and HEAD are permitted on metrics route.", res->is_keep_alive);
            return 0;
        }

        size_t q_depth = connection_queue_get_count(&ctx->queue);
        size_t active_w = thread_pool_get_active_count(&ctx->thread_pool);
        size_t total_w = thread_pool_get_total_count(&ctx->thread_pool);
        MetricsSnapshot snap;
        metrics_get_snapshot(&ctx->metrics, q_depth, active_w, total_w, &snap);

        const char *qlaac_state_str = adaptive_controller_state_name(ctx->controller.current_state);

        char json_buf[4096];
        int written = snprintf(json_buf, sizeof(json_buf),
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
            "  \"recent_latency_sample_count\": %zu,\n"
            "  \"qlaac_enabled\": %s,\n"
            "  \"qlaac_state\": \"%s\",\n"
            "  \"qlaac_overload_transitions\": %llu,\n"
            "  \"qlaac_shed_requests\": %llu,\n"
            "  \"queue_capacity\": %zu,\n"
            "  \"uptime_seconds\": %ld,\n"
            "  \"snapshot_timestamp\": %ld\n"
            "}\n",
            (unsigned long long)snap.total_connections_accepted,
            (unsigned long long)snap.total_requests_completed,
            (unsigned long long)snap.total_connections_rejected,
            (unsigned long long)snap.total_overload_503_sent,
            (unsigned long long)snap.status_2xx_count,
            (unsigned long long)snap.status_3xx_count,
            (unsigned long long)snap.status_4xx_count,
            (unsigned long long)snap.status_5xx_count,
            (unsigned long long)snap.bytes_received,
            (unsigned long long)snap.bytes_sent,
            snap.current_queue_depth,
            snap.active_workers,
            snap.total_workers,
            snap.requests_per_second,
            snap.lifetime_requests_per_second,
            snap.median_latency_ms,
            snap.p95_latency_ms,
            snap.p99_latency_ms,
            snap.avg_latency_ms,
            snap.recent_latency_sample_count,
            ctx->config.adaptive_control_enabled ? "true" : "false",
            qlaac_state_str,
            (unsigned long long)ctx->controller.total_overload_transitions,
            (unsigned long long)ctx->controller.total_shed_requests,
            ctx->config.connection_queue_capacity,
            (long)(snap.snapshot_timestamp - snap.server_start_time),
            (long)snap.snapshot_timestamp
        );

        if (written > 0 && (size_t)written < sizeof(json_buf)) {
            http_response_set_status(res, HTTP_STATUS_OK);
            http_response_set_body_copy(res, "application/json; charset=utf-8", json_buf, (size_t)written);
        } else {
            http_response_build_error(res, HTTP_STATUS_INTERNAL_ERROR,
                                      "Telemetry serialization overflow.", res->is_keep_alive);
        }
        return 0;
    }

    /* 3. Echo API Endpoint (POST Body Echo) */
    if (strncmp(req->uri, "/api/echo", 9) == 0 && (req->uri[9] == '\0' || req->uri[9] == '?' || req->uri[9] == '/')) {
        if (req->method != HTTP_METHOD_POST) {
            http_response_build_error(res, HTTP_STATUS_METHOD_NOT_ALLOWED,
                                      "POST is required for /api/echo.", res->is_keep_alive);
            return 0;
        }

        const char *content_type = http_request_get_header(req, "Content-Type");
        if (!content_type) {
            content_type = "text/plain; charset=utf-8";
        }

        http_response_set_status(res, HTTP_STATUS_OK);
        if (req->body && req->body_len > 0) {
            http_response_set_body_copy(res, content_type, req->body, req->body_len);
        } else {
            http_response_set_body_copy(res, content_type, "", 0);
        }
        return 0;
    }

    /* 4. Static Asset Delegation */
    if (req->method != HTTP_METHOD_GET && req->method != HTTP_METHOD_HEAD) {
        http_response_build_error(res, HTTP_STATUS_METHOD_NOT_ALLOWED,
                                  "Only GET and HEAD methods are permitted on static resources.", res->is_keep_alive);
        return 0;
    }

    return static_files_serve(ctx->config.public_root, req->uri, res);
}
