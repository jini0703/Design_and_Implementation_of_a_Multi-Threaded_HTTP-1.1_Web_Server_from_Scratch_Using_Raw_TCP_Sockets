/**
 * @file thread_pool.c
 * @brief Fixed-size POSIX pthread worker pool for concurrent HTTP connection processing.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Intended Responsibilities:
 *  1. Worker Lifecycle Management:
 *     - Spawn a deterministic number of worker threads at server startup.
 *     - Execute persistent worker loop continuously dequeuing from ConnectionQueue.
 *  2. Concurrency & Flow Control:
 *     - Eliminate per-connection fork/thread creation overhead.
 *     - Provide backpressure when all workers are saturated and queue reaches capacity.
 *  3. HTTP Connection Execution:
 *     - Read and parse HTTP requests over the accepted client socket.
 *     - Maintain sequential persistent connections (HTTP/1.1 keep-alive loop).
 *     - Dispatch requests to router.c, generate responses, and write back to socket.
 *  4. Descriptor Ownership & Safety:
 *     - Ensure unambiguous descriptor ownership: the worker thread alone reads, writes,
 *       and closes the client socket upon disconnect or timeout.
 *  5. Graceful Teardown:
 *     - Signal workers on shutdown, wake them via queue condition variables, and pthread_join().
 *
 * Implementation Status:
 *  - Architecture specified in include/thread_pool.h.
 *  - Worker thread processing loop scheduled for development in Phase 1.
 */

#include "thread_pool.h"

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
