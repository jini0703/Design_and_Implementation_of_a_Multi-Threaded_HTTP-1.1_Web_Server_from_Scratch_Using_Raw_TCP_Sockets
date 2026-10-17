/**
 * @file thread_pool.c
 * @brief Fixed-size POSIX pthread worker pool for concurrent HTTP connection processing.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include "thread_pool.h"
#include "server.h"
#include "router.h"
#include "http_parser.h"
#include "http_response.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void *worker_thread_routine(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    ServerContext *ctx = pool->server_ctx;

    while (pool->is_running) {
        ClientConnection conn;
        int pop_rc = connection_queue_pop(pool->queue, &conn, 1000);
        if (pop_rc == -2) {
            /* Queue is shut down */
            break;
        }
        if (pop_rc == -1) {
            /* Timeout, check running flag and continue */
            continue;
        }

        /* Mark worker active */
        pthread_mutex_lock(&pool->stats_lock);
        pool->active_workers++;
        pthread_mutex_unlock(&pool->stats_lock);

        /* Initialize streaming buffer for persistent HTTP connection */
        ConnectionBuffer cb;
        size_t initial_cap = ctx->config.io_buffer_size;
        if (initial_cap == 0) initial_cap = 8192;
        conn_buffer_init(&cb, initial_cap);

        int request_count = 0;
        int max_requests = ctx->config.keepalive_max_requests;
        int keepalive_timeout_sec = ctx->config.keepalive_timeout_sec;
        if (max_requests <= 0) max_requests = 100;
        if (keepalive_timeout_sec <= 0) keepalive_timeout_sec = 5;

        while (pool->is_running) {
            /* Check QLAAC controller admission state */
            QlaacDecision qdec = adaptive_controller_evaluate(&ctx->controller);
            if (qdec == QLAAC_ACTION_THROTTLE_KEEPALIVE) {
                keepalive_timeout_sec = 1;
                max_requests = 1;
            }

            uint64_t req_start_time = 0;
            HttpRequest req;
            http_request_init(&req);
            size_t bytes_consumed = 0;
            ParserStatus status = PARSER_NEED_MORE_DATA;

            if (cb.length > 0) {
                req_start_time = monotonic_ns();
                status = http_parse_request(cb.buf, cb.length,
                                            ctx->config.max_header_size,
                                            ctx->config.max_body_size,
                                            &req, &bytes_consumed);
            }

            uint64_t deadline = monotonic_ns() + (uint64_t)keepalive_timeout_sec * 1000000000ULL;

            while (status == PARSER_NEED_MORE_DATA && pool->is_running) {
                if (cb.length == cb.capacity) {
                    size_t max_allowed = ctx->config.max_header_size + ctx->config.max_body_size + 4096;
                    size_t new_cap = cb.capacity * 2;
                    if (new_cap > max_allowed) new_cap = max_allowed;
                    if (new_cap <= cb.capacity) {
                        status = PARSER_ERR_BODY_TOO_LARGE;
                        break;
                    }
                    char *new_buf = (char *)realloc(cb.buf, new_cap);
                    if (!new_buf) {
                        status = PARSER_ERR_BODY_TOO_LARGE;
                        break;
                    }
                    cb.buf = new_buf;
                    cb.capacity = new_cap;
                }

                ssize_t n = tcp_recv_some(conn.fd, cb.buf + cb.length, cb.capacity - cb.length,
                                          deadline, NULL);
                if (n == 0) {
                    /* Orderly peer disconnect */
                    break;
                }
                if (n < 0) {
                    /* Read timeout or transport reset */
                    break;
                }

                if (req_start_time == 0) {
                    req_start_time = monotonic_ns();
                }

                cb.length += (size_t)n;
                status = http_parse_request(cb.buf, cb.length,
                                            ctx->config.max_header_size,
                                            ctx->config.max_body_size,
                                            &req, &bytes_consumed);
            }

            if (status == PARSER_NEED_MORE_DATA) {
                http_request_destroy(&req);
                break;
            }

            if (req_start_time == 0) {
                req_start_time = monotonic_ns();
            }

            if (status != PARSER_OK) {
                int http_code = parser_status_to_http_code(status);
                HttpResponse err_res;
                http_response_init(&err_res);
                err_res.is_keep_alive = false;
                http_response_build_error(&err_res, (HttpStatus)http_code,
                                          "Protocol framing or syntax error.", false);

                uint64_t send_deadline = monotonic_ns() + (uint64_t)ctx->config.socket_snd_timeout_sec * 1000000000ULL;
                size_t sent = 0;
                http_response_send(conn.fd, &err_res, send_deadline, false, &sent);

                uint64_t req_end = monotonic_ns();
                double lat = (double)(req_end - req_start_time) / 1000000.0;
                metrics_record_request(&ctx->metrics, http_code, lat, cb.length, sent);

                http_response_destroy(&err_res);
                http_request_destroy(&req);
                break;
            }

            /* Shift unconsumed pipelined bytes */
            conn_buffer_consume(&cb, bytes_consumed);
            request_count++;

            if (request_count >= max_requests) {
                req.keep_alive = false;
            }

            HttpResponse res;
            http_response_init(&res);
            res.is_keep_alive = req.keep_alive;

            router_dispatch(ctx, &req, &res);

            uint64_t send_deadline = monotonic_ns() + (uint64_t)ctx->config.socket_snd_timeout_sec * 1000000000ULL;
            size_t sent = 0;
            int send_rc = http_response_send(conn.fd, &res, send_deadline,
                                             ctx->config.enable_sendfile, &sent);

            uint64_t req_end = monotonic_ns();
            double lat = (double)(req_end - req_start_time) / 1000000.0;
            const char *m_route = (ctx->config.metrics_route[0] != '\0') ? ctx->config.metrics_route : "/metrics";
            if (strcmp(req.uri, m_route) == 0 || strcmp(req.uri, "/metrics") == 0) {
                lat = -1.0;
            }
            metrics_record_request(&ctx->metrics, res.status, lat, bytes_consumed, sent);

            bool persist = res.is_keep_alive && (send_rc == 0) && (request_count < max_requests);

            http_response_destroy(&res);
            http_request_destroy(&req);

            if (!persist) {
                break;
            }
        }

        conn_buffer_free(&cb);
        tcp_lingering_close(&conn.fd, 200);

        /* Mark worker inactive */
        pthread_mutex_lock(&pool->stats_lock);
        pool->active_workers--;
        pthread_mutex_unlock(&pool->stats_lock);
    }

    return NULL;
}

int thread_pool_init(ThreadPool *pool, size_t thread_count,
                     ConnectionQueue *queue, ServerContext *server_ctx) {
    if (!pool || thread_count == 0 || !queue || !server_ctx) return -1;

    pool->thread_count = thread_count;
    pool->queue = queue;
    pool->server_ctx = server_ctx;
    pool->active_workers = 0;
    pool->is_running = true;

    if (pthread_mutex_init(&pool->stats_lock, NULL) != 0) {
        return -1;
    }

    pool->threads = (pthread_t *)calloc(thread_count, sizeof(pthread_t));
    if (!pool->threads) {
        pthread_mutex_destroy(&pool->stats_lock);
        return -1;
    }

    for (size_t i = 0; i < thread_count; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread_routine, pool) != 0) {
            pool->is_running = false;
            for (size_t j = 0; j < i; j++) {
                pthread_join(pool->threads[j], NULL);
            }
            free(pool->threads);
            pool->threads = NULL;
            pthread_mutex_destroy(&pool->stats_lock);
            return -1;
        }
    }

    return 0;
}

size_t thread_pool_get_active_count(ThreadPool *pool) {
    if (!pool) return 0;
    pthread_mutex_lock(&pool->stats_lock);
    size_t active = pool->active_workers;
    pthread_mutex_unlock(&pool->stats_lock);
    return active;
}

size_t thread_pool_get_total_count(const ThreadPool *pool) {
    return pool ? pool->thread_count : 0;
}

void thread_pool_shutdown(ThreadPool *pool) {
    if (!pool || !pool->threads) return;

    pool->is_running = false;
    connection_queue_shutdown(pool->queue);

    for (size_t i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    free(pool->threads);
    pool->threads = NULL;
    pthread_mutex_destroy(&pool->stats_lock);
}
