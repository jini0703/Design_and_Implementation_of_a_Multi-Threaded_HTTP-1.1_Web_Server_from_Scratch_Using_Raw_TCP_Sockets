#ifndef SERVER_H
#define SERVER_H

/**
 * @file server.h
 * @brief Top-level server lifecycle management, configuration, and event loop.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stddef.h>
#include "tcp_socket.h"
#include "connection_queue.h"
#include "thread_pool.h"
#include "metrics.h"
#include "adaptive_controller.h"

#define SERVER_VERSION_STRING "HTTP-Server-C17/1.0.0"
#define DEFAULT_CONFIG_FILE   "config/server.conf.example"

/**
 * @brief Server configuration parameters parsed from file or CLI defaults.
 */
typedef struct {
    char host[64];
    uint16_t port;
    int backlog;
    int socket_rcv_timeout_sec;
    int socket_snd_timeout_sec;
    size_t worker_threads;
    size_t connection_queue_capacity;
    int worker_idle_timeout_sec;
    int keepalive_max_requests;
    int keepalive_timeout_sec;
    size_t max_header_size;
    size_t max_body_size;
    size_t io_buffer_size;
    char public_root[256];
    char default_index[64];
    bool enable_sendfile;
    bool adaptive_control_enabled;
    int sampling_window_ms;
    size_t recovery_sample_count;
    size_t sustained_pressure_threshold;
    double latency_caution_ms;
    double latency_overload_ms;
    double queue_wait_caution_ms;
    double queue_wait_overload_ms;
    double queue_caution_threshold;
    double queue_overload_threshold;
    double worker_caution_threshold;
    double worker_overload_threshold;
    bool metrics_enabled;
    char metrics_route[64];
} ServerConfig;

/**
 * @brief Primary server runtime context encapsulating all subsystems.
 */
struct ServerContext {
    ServerConfig config;
    int listen_fd;
    ConnectionQueue queue;
    ThreadPool thread_pool;
    MetricsCollector metrics;
    QlaacController controller;
    atomic_bool is_running;
    pthread_t sampler_thread;
    bool sampler_thread_started;
};

/**
 * @brief Populate server configuration with safe default values.
 * @param config Pointer to configuration struct to initialize.
 */
void server_config_init_defaults(ServerConfig *config);

/**
 * @brief Load configuration parameters from a key-value file.
 * @param config Pointer to configuration struct.
 * @param filepath Path to the configuration file.
 * @return 0 on success, negative error code on failure.
 */
int server_config_load_file(ServerConfig *config, const char *filepath);

/**
 * @brief Initialize all server subsystems based on provided configuration.
 * @param config Configuration parameters.
 * @return Allocated ServerContext pointer on success, NULL on error.
 */
ServerContext *server_init(const ServerConfig *config);

/**
 * @brief Run the primary server listening loop (blocking until termination signal).
 * @param ctx Active server context.
 * @return 0 on clean exit, non-zero on error.
 */
int server_run(ServerContext *ctx);

/**
 * @brief Signal the server context to cease accepting new connections and drain workers.
 * @param ctx Active server context.
 */
void server_request_shutdown(ServerContext *ctx);

/**
 * @brief Async-signal-safe request for server shutdown (safe to call from signal handlers).
 */
void server_signal_request_shutdown(void);

/**
 * @brief Query if a shutdown was requested asynchronously via signal.
 * @return True if shutdown requested, false otherwise.
 */
bool server_is_shutdown_requested(void);

/**
 * @brief Cleanly tear down thread pool, sockets, metrics, and free runtime context.
 * @param ctx Server context to destroy.
 */
void server_destroy(ServerContext *ctx);

#endif /* SERVER_H */
