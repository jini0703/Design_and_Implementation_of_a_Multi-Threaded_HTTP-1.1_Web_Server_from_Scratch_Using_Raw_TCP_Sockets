/**
 * @file server.c
 * @brief Top-level server lifecycle orchestrator and main connection accept loop.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include "server.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <poll.h>
#include <unistd.h>

static char *trim_whitespace(char *str) {
    if (!str) return NULL;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void server_config_init_defaults(ServerConfig *config) {
    if (!config) return;
    strncpy(config->host, "0.0.0.0", sizeof(config->host) - 1);
    config->host[sizeof(config->host) - 1] = '\0';
    config->port = 8080;
    config->backlog = 128;
    config->socket_rcv_timeout_sec = 5;
    config->socket_snd_timeout_sec = 5;
    config->worker_threads = 4;
    config->connection_queue_capacity = 64;
    config->keepalive_max_requests = 100;
    config->keepalive_timeout_sec = 5;
    config->max_header_size = 16384;
    config->max_body_size = 10485760;
    config->io_buffer_size = 16384;
    strncpy(config->public_root, "public", sizeof(config->public_root) - 1);
    config->public_root[sizeof(config->public_root) - 1] = '\0';
    strncpy(config->default_index, "index.html", sizeof(config->default_index) - 1);
    config->default_index[sizeof(config->default_index) - 1] = '\0';
    config->enable_sendfile = true;
    config->adaptive_control_enabled = false;
    config->sampling_window_ms = 1000;
    config->recovery_sample_count = 5;
    config->sustained_pressure_threshold = 3;
    config->latency_caution_ms = 50.0;
    config->latency_overload_ms = 150.0;
    config->queue_caution_threshold = 0.70;
    config->queue_overload_threshold = 0.90;
    config->worker_caution_threshold = 0.85;
    config->worker_overload_threshold = 1.00;
    config->metrics_enabled = true;
    strncpy(config->metrics_route, "/metrics", sizeof(config->metrics_route) - 1);
    config->metrics_route[sizeof(config->metrics_route) - 1] = '\0';
}

int server_config_load_file(ServerConfig *config, const char *filepath) {
    if (!config || !filepath) return -1;

    FILE *f = fopen(filepath, "r");
    if (!f) return -1;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *p = trim_whitespace(line);
        if (*p == '\0' || *p == '#' || *p == ';') {
            continue;
        }

        char *eq = strchr(p, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = trim_whitespace(p);
        char *val = trim_whitespace(eq + 1);

        if (strcasecmp(key, "host") == 0) {
            strncpy(config->host, val, sizeof(config->host) - 1);
            config->host[sizeof(config->host) - 1] = '\0';
        } else if (strcasecmp(key, "port") == 0) {
            config->port = (uint16_t)atoi(val);
        } else if (strcasecmp(key, "socket_backlog") == 0 || strcasecmp(key, "backlog") == 0) {
            config->backlog = atoi(val);
        } else if (strcasecmp(key, "socket_rcv_timeout_sec") == 0) {
            config->socket_rcv_timeout_sec = atoi(val);
        } else if (strcasecmp(key, "socket_snd_timeout_sec") == 0) {
            config->socket_snd_timeout_sec = atoi(val);
        } else if (strcasecmp(key, "worker_threads") == 0) {
            config->worker_threads = (size_t)atoi(val);
        } else if (strcasecmp(key, "connection_queue_capacity") == 0) {
            config->connection_queue_capacity = (size_t)atoi(val);
        } else if (strcasecmp(key, "keepalive_max_requests") == 0) {
            config->keepalive_max_requests = atoi(val);
        } else if (strcasecmp(key, "keepalive_timeout_sec") == 0) {
            config->keepalive_timeout_sec = atoi(val);
        } else if (strcasecmp(key, "max_header_size") == 0) {
            config->max_header_size = (size_t)strtoul(val, NULL, 10);
        } else if (strcasecmp(key, "max_body_size") == 0) {
            config->max_body_size = (size_t)strtoul(val, NULL, 10);
        } else if (strcasecmp(key, "io_buffer_size") == 0) {
            config->io_buffer_size = (size_t)strtoul(val, NULL, 10);
        } else if (strcasecmp(key, "public_root") == 0) {
            strncpy(config->public_root, val, sizeof(config->public_root) - 1);
            config->public_root[sizeof(config->public_root) - 1] = '\0';
        } else if (strcasecmp(key, "default_index") == 0) {
            strncpy(config->default_index, val, sizeof(config->default_index) - 1);
            config->default_index[sizeof(config->default_index) - 1] = '\0';
        } else if (strcasecmp(key, "enable_sendfile") == 0) {
            config->enable_sendfile = (strcasecmp(val, "true") == 0 || strcmp(val, "1") == 0);
        } else if (strcasecmp(key, "adaptive_control_enabled") == 0) {
            config->adaptive_control_enabled = (strcasecmp(val, "true") == 0 || strcmp(val, "1") == 0);
        } else if (strcasecmp(key, "sampling_window_ms") == 0) {
            config->sampling_window_ms = atoi(val);
        } else if (strcasecmp(key, "recovery_sample_count") == 0) {
            config->recovery_sample_count = (size_t)strtoul(val, NULL, 10);
        } else if (strcasecmp(key, "sustained_pressure_threshold") == 0) {
            config->sustained_pressure_threshold = (size_t)strtoul(val, NULL, 10);
        } else if (strcasecmp(key, "queue_caution_threshold") == 0) {
            config->queue_caution_threshold = atof(val);
        } else if (strcasecmp(key, "queue_overload_threshold") == 0) {
            config->queue_overload_threshold = atof(val);
        } else if (strcasecmp(key, "latency_caution_ms") == 0) {
            config->latency_caution_ms = atof(val);
        } else if (strcasecmp(key, "latency_overload_ms") == 0) {
            config->latency_overload_ms = atof(val);
        } else if (strcasecmp(key, "worker_caution_threshold") == 0) {
            config->worker_caution_threshold = atof(val);
        } else if (strcasecmp(key, "worker_overload_threshold") == 0) {
            config->worker_overload_threshold = atof(val);
        } else if (strcasecmp(key, "metrics_enabled") == 0) {
            config->metrics_enabled = (strcasecmp(val, "true") == 0 || strcmp(val, "1") == 0);
        } else if (strcasecmp(key, "metrics_route") == 0) {
            strncpy(config->metrics_route, val, sizeof(config->metrics_route) - 1);
            config->metrics_route[sizeof(config->metrics_route) - 1] = '\0';
        }
    }

    fclose(f);
    return 0;
}

ServerContext *server_init(const ServerConfig *config) {
    ServerContext *ctx = (ServerContext *)calloc(1, sizeof(ServerContext));
    if (!ctx) return NULL;

    if (config) {
        ctx->config = *config;
    } else {
        server_config_init_defaults(&ctx->config);
    }

    /* Initialize metrics collector */
    metrics_init(&ctx->metrics);

    /* Initialize QLAAC adaptive controller */
    QlaacConfig q_cfg;
    q_cfg.enabled = ctx->config.adaptive_control_enabled;
    q_cfg.queue_caution_threshold = ctx->config.queue_caution_threshold;
    q_cfg.queue_overload_threshold = ctx->config.queue_overload_threshold;
    q_cfg.latency_caution_ms = ctx->config.latency_caution_ms;
    q_cfg.latency_overload_ms = ctx->config.latency_overload_ms;
    q_cfg.worker_caution_threshold = ctx->config.worker_caution_threshold;
    q_cfg.worker_overload_threshold = ctx->config.worker_overload_threshold;
    q_cfg.recovery_samples_required = (ctx->config.recovery_sample_count > 0) ? ctx->config.recovery_sample_count : 5;
    q_cfg.sustained_pressure_threshold = (ctx->config.sustained_pressure_threshold > 0) ? ctx->config.sustained_pressure_threshold : 3;
    adaptive_controller_init(&ctx->controller, &q_cfg);

    /* Initialize bounded connection queue */
    if (connection_queue_init(&ctx->queue, ctx->config.connection_queue_capacity) != 0) {
        log_msg(LOG_LEVEL_ERROR, "Failed to initialize bounded connection queue.");
        metrics_destroy(&ctx->metrics);
        adaptive_controller_destroy(&ctx->controller);
        free(ctx);
        return NULL;
    }

    /* Create non-blocking TCP stream listener */
    char err_msg[256];
    ctx->listen_fd = tcp_listen_create(ctx->config.host, ctx->config.port, ctx->config.backlog,
                                       err_msg, sizeof(err_msg));
    if (ctx->listen_fd < 0) {
        log_msg(LOG_LEVEL_ERROR, "Socket listener creation failed: %s", err_msg);
        connection_queue_destroy(&ctx->queue);
        metrics_destroy(&ctx->metrics);
        adaptive_controller_destroy(&ctx->controller);
        free(ctx);
        return NULL;
    }

    ctx->is_running = true;

    /* Initialize fixed worker thread pool */
    if (thread_pool_init(&ctx->thread_pool, ctx->config.worker_threads, &ctx->queue, ctx) != 0) {
        log_msg(LOG_LEVEL_ERROR, "Failed to initialize worker thread pool.");
        tcp_close(&ctx->listen_fd);
        connection_queue_destroy(&ctx->queue);
        metrics_destroy(&ctx->metrics);
        adaptive_controller_destroy(&ctx->controller);
        free(ctx);
        return NULL;
    }

    log_msg(LOG_LEVEL_INFO, "Server initialized on %s:%u (workers=%zu, queue_cap=%zu, qlaac=%s)",
            ctx->config.host, ctx->config.port,
            ctx->config.worker_threads, ctx->config.connection_queue_capacity,
            ctx->config.adaptive_control_enabled ? "ENABLED" : "DISABLED");

    return ctx;
}

static void *sampler_thread_func(void *arg) {
    ServerContext *ctx = (ServerContext *)arg;
    int sampling_ms = ctx->config.sampling_window_ms > 0 ? ctx->config.sampling_window_ms : 1000;

    log_msg(LOG_LEVEL_INFO, "[SAMPLER] Dedicated telemetry sampler thread started (interval=%d ms)", sampling_ms);

    while (ctx->is_running) {
        /* Sleep in short 50ms increments so shutdown is immediate */
        int remaining_ms = sampling_ms;
        while (ctx->is_running && remaining_ms > 0) {
            int slice = remaining_ms > 50 ? 50 : remaining_ms;
            usleep(slice * 1000);
            remaining_ms -= slice;
        }

        if (!ctx->is_running) break;

        size_t q_depth = connection_queue_get_count(&ctx->queue);
        double q_ratio = connection_queue_occupancy_ratio(&ctx->queue);
        size_t active_w = thread_pool_get_active_count(&ctx->thread_pool);
        size_t total_w = thread_pool_get_total_count(&ctx->thread_pool);
        double worker_util = (total_w > 0) ? ((double)active_w / (double)total_w) : 0.0;

        MetricsSnapshot snap;
        metrics_get_snapshot(&ctx->metrics, q_depth, active_w, total_w, &snap);

        if (ctx->config.adaptive_control_enabled) {
            adaptive_controller_update_ex(&ctx->controller, q_ratio, worker_util, snap.p95_latency_ms);
        }
    }

    log_msg(LOG_LEVEL_INFO, "[SAMPLER] Dedicated telemetry sampler thread exiting.");
    return NULL;
}

int server_run(ServerContext *ctx) {
    if (!ctx || ctx->listen_fd < 0) return -1;

    /* Launch dedicated periodic telemetry sampler thread */
    ctx->sampler_thread_started = false;
    if (pthread_create(&ctx->sampler_thread, NULL, sampler_thread_func, ctx) == 0) {
        ctx->sampler_thread_started = true;
    } else {
        log_msg(LOG_LEVEL_ERROR, "Failed to launch periodic metrics sampler thread.");
    }

    log_msg(LOG_LEVEL_INFO, "Entering master accept event loop on port %u...", ctx->config.port);

    while (ctx->is_running) {
        struct pollfd pfd;
        pfd.fd = ctx->listen_fd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        int poll_rc = poll(&pfd, 1, 100);
        if (poll_rc <= 0) {
            /* Timeout or interrupted: loop back to check ctx->is_running */
            continue;
        }

        if (!(pfd.revents & POLLIN)) {
            continue;
        }

        ClientConnection client;
        int err_num = 0;
        TcpAcceptResult res = tcp_accept_client(ctx->listen_fd, &client, &err_num);

        if (res == TCP_ACCEPT_AGAIN || res == TCP_ACCEPT_RETRY) {
            continue;
        }

        if (res == TCP_ACCEPT_RESOURCE) {
            /* File descriptor limit reached (EMFILE / ENFILE) */
            log_msg(LOG_LEVEL_WARN, "Descriptor table exhausted (EMFILE/ENFILE); sleeping 20ms");
            usleep(20000);
            continue;
        }

        if (res == TCP_ACCEPT_FATAL) {
            if (ctx->is_running) {
                log_msg(LOG_LEVEL_ERROR, "Fatal accept error on listening socket.");
            }
            break;
        }

        if (res == TCP_ACCEPT_OK) {
            metrics_record_connection_accepted(&ctx->metrics);

            /* Evaluate QLAAC overload admission state */
            QlaacDecision decision = adaptive_controller_evaluate(&ctx->controller);
            if (decision == QLAAC_ACTION_REJECT_OVERLOAD) {
                metrics_record_connection_rejected(&ctx->metrics);
                const char *msg_503 =
                    "HTTP/1.1 503 Service Unavailable\r\n"
                    "Content-Type: text/html; charset=utf-8\r\n"
                    "Content-Length: 96\r\n"
                    "Connection: close\r\n"
                    "Retry-After: 5\r\n"
                    "\r\n"
                    "<html><body><h1>503 Service Unavailable</h1><p>Server under extreme overload.</p></body></html>\n";

                size_t sent_bytes = 0;
                uint64_t deadline = monotonic_ns() + 100 * NS_PER_MS;
                int send_rc = tcp_send_all(client.fd, msg_503, strlen(msg_503), deadline, &sent_bytes);
                if (send_rc == 0) {
                    metrics_record_overload_response(&ctx->metrics, 503, sent_bytes);
                }
                tcp_lingering_close(&client.fd, 50);
                continue;
            }

            /* Attempt non-blocking enqueue into bounded queue */
            int push_rc = connection_queue_push(&ctx->queue, &client, 0);
            if (push_rc != 0) {
                /* Queue capacity exhausted or shutting down */
                metrics_record_connection_rejected(&ctx->metrics);
                const char *msg_queue_full =
                    "HTTP/1.1 503 Service Unavailable\r\n"
                    "Content-Type: text/html; charset=utf-8\r\n"
                    "Content-Length: 98\r\n"
                    "Connection: close\r\n"
                    "Retry-After: 2\r\n"
                    "\r\n"
                    "<html><body><h1>503 Service Unavailable</h1><p>Server queue is fully saturated.</p></body></html>\n";

                size_t sent_bytes = 0;
                uint64_t deadline = monotonic_ns() + 100 * NS_PER_MS;
                int send_rc = tcp_send_all(client.fd, msg_queue_full, strlen(msg_queue_full), deadline, &sent_bytes);
                if (send_rc == 0) {
                    metrics_record_overload_response(&ctx->metrics, 503, sent_bytes);
                }
                tcp_lingering_close(&client.fd, 50);
                continue;
            }
        }
    }

    log_msg(LOG_LEVEL_INFO, "Exiting master accept loop.");
    return 0;
}

void server_request_shutdown(ServerContext *ctx) {
    if (!ctx) return;
    ctx->is_running = false;
    connection_queue_shutdown(&ctx->queue);
}

void server_destroy(ServerContext *ctx) {
    if (!ctx) return;

    log_msg(LOG_LEVEL_INFO, "Initiating graceful shutdown of all subsystems...");
    server_request_shutdown(ctx);

    if (ctx->sampler_thread_started) {
        pthread_join(ctx->sampler_thread, NULL);
        ctx->sampler_thread_started = false;
    }

    if (ctx->listen_fd >= 0) {
        tcp_close(&ctx->listen_fd);
    }

    thread_pool_shutdown(&ctx->thread_pool);
    connection_queue_destroy(&ctx->queue);
    metrics_destroy(&ctx->metrics);
    adaptive_controller_destroy(&ctx->controller);

    log_msg(LOG_LEVEL_INFO, "Server subsystems teardown complete.");
    free(ctx);
}
