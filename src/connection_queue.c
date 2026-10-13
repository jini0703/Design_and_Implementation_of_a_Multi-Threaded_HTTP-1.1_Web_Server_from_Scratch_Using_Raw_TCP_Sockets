/**
 * @file connection_queue.c
 * @brief Thread-safe bounded connection queue implementation using POSIX synchronization.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Intended Responsibilities:
 *  1. Bounded Buffer Management:
 *     - Maintain a fixed-size circular array of ClientConnection objects.
 *     - Prevent unbounded queue expansion and memory exhaustion during high-concurrency bursts.
 *  2. Concurrency Control:
 *     - Coordinate producers (main accept thread) and consumers (worker thread pool).
 *     - Synchronize critical sections with pthread_mutex_t.
 *     - Manage flow control with pthread_cond_t (queue_not_empty and queue_not_full).
 *  3. Overload Interaction:
 *     - Expose non-blocking occupancy checks (connection_queue_occupancy_ratio)
 *       to feed the QLAAC adaptive admission controller.
 *  4. Graceful Teardown:
 *     - Set shutdown flag and invoke pthread_cond_broadcast() to awaken all blocked threads.
 *     - Safely drain remaining descriptors and release buffer memory.
 *
 * Implementation Status:
 *  - Architecture specified in include/connection_queue.h.
 *  - Synchronization logic scheduled for development in Phase 1.
 */

#include "connection_queue.h"
#include <stdlib.h>

int connection_queue_init(ConnectionQueue *queue, size_t capacity) {
    if (!queue || capacity == 0) return -1;
    queue->buffer = (ClientConnection *)calloc(capacity, sizeof(ClientConnection));
    if (!queue->buffer) return -1;

    queue->capacity = capacity;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->is_shutdown = false;
    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
    pthread_cond_init(&queue->not_full, NULL);
    return 0;
}

size_t connection_queue_get_count(ConnectionQueue *queue) {
    if (!queue) return 0;
    pthread_mutex_lock(&queue->lock);
    size_t count = queue->count;
    pthread_mutex_unlock(&queue->lock);
    return count;
}

double connection_queue_occupancy_ratio(ConnectionQueue *queue) {
    if (!queue) return 0.0;
    pthread_mutex_lock(&queue->lock);
    double ratio = (queue->capacity > 0) ? ((double)queue->count / (double)queue->capacity) : 0.0;
    pthread_mutex_unlock(&queue->lock);
    return ratio;
}

void connection_queue_shutdown(ConnectionQueue *queue) {
    if (!queue) return;
    pthread_mutex_lock(&queue->lock);
    queue->is_shutdown = true;
    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);
    pthread_mutex_unlock(&queue->lock);
}

void connection_queue_destroy(ConnectionQueue *queue) {
    if (!queue) return;
    connection_queue_shutdown(queue);
    pthread_mutex_lock(&queue->lock);
    free(queue->buffer);
    queue->buffer = NULL;
    pthread_mutex_unlock(&queue->lock);
    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
}
