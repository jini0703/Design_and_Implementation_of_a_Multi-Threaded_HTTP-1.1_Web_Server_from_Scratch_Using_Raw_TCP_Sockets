#ifndef SERVER_H
#define SERVER_H

/**
 * @file server.h
 * @brief Top-level server lifecycle management, configuration, and event loop.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Module Responsibilities:
 *  - Encapsulate server configuration and runtime context.
 *  - Orchestrate initialization of network listener, thread pool, connection queue,
 *    metrics collector, and adaptive controller.
 *  - Execute the master connection accept loop.
 *  - Handle OS signals (SIGINT, SIGTERM) to initiate clean, graceful shutdown.
 *  - Reclaim all allocated memory, socket descriptors, and synchronization primitives.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define SERVER_VERSION_STRING "HTTP-Server-Skeleton/0.1.0"
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
    int keepalive_max_requests;
    int keepalive_timeout_sec;
    size_t max_header_size;
    size_t max_body_size;
    size_t io_buffer_size;
    char public_root[256];
    char default_index[64];
    bool enable_sendfile;
    bool adaptive_control_enabled;
    double latency_caution_ms;
    double latency_overload_ms;
    double queue_caution_threshold;
    double queue_overload_threshold;
    bool metrics_enabled;
    char metrics_route[64];
} ServerConfig;

/**
 * @brief Opaque server runtime context structure.
 */
typedef struct ServerContext ServerContext;

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
 * @brief Cleanly tear down thread pool, sockets, metrics, and free runtime context.
 * @param ctx Server context to destroy.
 */
void server_destroy(ServerContext *ctx);

#endif /* SERVER_H */
