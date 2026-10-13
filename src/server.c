/**
 * @file server.c
 * @brief Top-level server lifecycle orchestrator and main connection accept loop.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Intended Responsibilities:
 *  1. Configuration Management:
 *     - Parse configuration files (e.g. config/server.conf.example) and populate ServerConfig.
 *     - Apply defensive boundary limits for socket backlog, buffer sizes, and thread limits.
 *  2. Subsystem Initialization:
 *     - Instantiate TCP socket listener via tcp_listen_create().
 *     - Initialize synchronized ConnectionQueue with configured bounded capacity.
 *     - Initialize and start ThreadPool with fixed worker threads.
 *     - Initialize MetricsCollector and optional QLAAC AdaptiveController.
 *  3. Connection Accept Loop:
 *     - Execute master listen loop calling tcp_accept_client().
 *     - Query QLAAC controller and bounded queue state for admission decisions.
 *     - Enqueue admitted client connections or gracefully reject with HTTP 503 if overloaded.
 *  4. Graceful Shutdown & Teardown:
 *     - Intercept POSIX signals (SIGINT, SIGTERM) using sigaction.
 *     - Broadcast termination signal to worker pool and connection queue.
 *     - Reclaim memory, close open socket descriptors, and join worker threads without leaks.
 *
 * Implementation Status:
 *  - Architecture and interface specification complete in include/server.h.
 *  - Core networking and thread pool integration scheduled for implementation in Phase 1.
 */

#include "server.h"

void server_config_init_defaults(ServerConfig *config) {
    if (!config) return;
    /* Sensible defaults defined for safe initial configuration */
    config->port = 8080;
    config->backlog = 128;
    config->socket_rcv_timeout_sec = 5;
    config->socket_snd_timeout_sec = 5;
    config->worker_threads = 4;
    config->connection_queue_capacity = 64;
    config->keepalive_max_requests = 100;
    config->keepalive_timeout_sec = 5;
    config->max_header_size = 8192;
    config->max_body_size = 10485760;
    config->io_buffer_size = 8192;
    config->enable_sendfile = true;
    config->adaptive_control_enabled = false;
    config->latency_caution_ms = 50.0;
    config->latency_overload_ms = 150.0;
    config->queue_caution_threshold = 0.70;
    config->queue_overload_threshold = 0.90;
    config->metrics_enabled = true;
}
