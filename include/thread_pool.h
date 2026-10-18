#ifndef THREAD_POOL_H
#define THREAD_POOL_H

/**
 * @file thread_pool.h
 * @brief POSIX pthread worker pool for concurrent HTTP connection handling.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Module Responsibilities:
 *  - Manage a fixed-size pool of worker threads spawned at server startup.
 *  - Continuously dequeue connections from the bounded ConnectionQueue.
 *  - Delegate HTTP request parsing, routing, and response transmission to worker threads.
 *  - Track active worker count atomically / with mutex synchronization for telemetry.
 *  - Coordinate clean thread cancellation or graceful draining upon server termination.
 *  - Guarantee that socket descriptors are strictly owned and closed by the processing worker.
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include "connection_queue.h"

/* Forward declaration of server context */
typedef struct ServerContext ServerContext;

/**
 * @brief Thread Pool management structure.
 */
typedef struct {
    pthread_t *threads;
    size_t thread_count;
    ConnectionQueue *queue;
    ServerContext *server_ctx;
    size_t active_workers;
    pthread_mutex_t stats_lock;
    atomic_bool is_running;
    atomic_bool stop_requested;
} ThreadPool;

/**
 * @brief Initialize and spawn worker threads.
 * @param pool Thread pool struct to initialize.
 * @param thread_count Number of worker threads to spawn.
 * @param queue Shared bounded connection queue.
 * @param server_ctx Server context passed to workers for routing and configuration.
 * @return 0 on success, negative error code on thread creation failure.
 */
int thread_pool_init(ThreadPool *pool, size_t thread_count,
                     ConnectionQueue *queue, ServerContext *server_ctx);

/**
 * @brief Query the number of worker threads currently processing client requests.
 * @param pool Thread pool.
 * @return Active worker thread count.
 */
size_t thread_pool_get_active_count(ThreadPool *pool);

/**
 * @brief Query total worker pool capacity.
 * @param pool Thread pool.
 * @return Total threads in the pool.
 */
size_t thread_pool_get_total_count(const ThreadPool *pool);

/**
 * @brief Request worker threads to stop processing persistent sockets promptly.
 * @param pool Thread pool.
 */
void thread_pool_request_stop(ThreadPool *pool);

/**
 * @brief Initiate shutdown of worker pool and join all spawned threads.
 * @param pool Thread pool to stop and clean up.
 */
void thread_pool_shutdown(ThreadPool *pool);

#endif /* THREAD_POOL_H */
