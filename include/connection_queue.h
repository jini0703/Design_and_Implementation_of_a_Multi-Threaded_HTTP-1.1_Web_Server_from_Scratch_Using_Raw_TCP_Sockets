#ifndef CONNECTION_QUEUE_H
#define CONNECTION_QUEUE_H

/**
 * @file connection_queue.h
 * @brief Thread-safe bounded connection queue using POSIX mutexes and condition variables.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Module Responsibilities:
 *  - Implement a bounded FIFO circular buffer storing accepted client socket descriptors.
 *  - Enforce strict upper capacity limits to prevent unbounded memory growth during traffic bursts.
 *  - Synchronize multi-threaded access using pthread_mutex_t and two pthread_cond_t variables
 *    (queue_not_empty and queue_not_full).
 *  - Support blocking enqueues/dequeues with timeout support to facilitate responsive shutdown.
 *  - Provide thread-safe inspection of queue depth and capacity for runtime metrics and admission control.
 */

#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include "tcp_socket.h"

/**
 * @brief Bounded connection queue synchronization structure.
 */
typedef struct {
    ClientConnection *buffer;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
    bool is_shutdown;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} ConnectionQueue;

/**
 * @brief Allocate and initialize a bounded connection queue.
 * @param queue Pointer to queue structure.
 * @param capacity Maximum number of simultaneous queued connections.
 * @return 0 on success, negative error code on failure.
 */
int connection_queue_init(ConnectionQueue *queue, size_t capacity);

/**
 * @brief Push an accepted connection into the queue (blocking if full, up to timeout).
 * @param queue Target queue.
 * @param conn Client connection to enqueue.
 * @param timeout_ms Max milliseconds to wait if queue is full (0 for non-blocking).
 * @return 0 on success, -1 if queue is full / timeout elapsed, -2 if queue is shut down.
 */
int connection_queue_push(ConnectionQueue *queue, const ClientConnection *conn, int timeout_ms);

/**
 * @brief Pop a connection from the queue for worker processing (blocking if empty).
 * @param queue Target queue.
 * @param conn Pointer to receive popped ClientConnection.
 * @param timeout_ms Max milliseconds to wait if queue is empty (0 for non-blocking).
 * @return 0 on success, -1 on timeout, -2 if queue is shut down.
 */
int connection_queue_pop(ConnectionQueue *queue, ClientConnection *conn, int timeout_ms);

/**
 * @brief Get current number of pending connections in the queue (thread-safe).
 * @param queue Target queue.
 * @return Current count of queued connections.
 */
size_t connection_queue_get_count(ConnectionQueue *queue);

/**
 * @brief Calculate current queue occupancy ratio (count / capacity).
 * @param queue Target queue.
 * @return Ratio between 0.0 (empty) and 1.0 (full).
 */
double connection_queue_occupancy_ratio(ConnectionQueue *queue);

/**
 * @brief Signal all waiting threads and transition queue into shutdown state.
 * @param queue Target queue.
 */
void connection_queue_shutdown(ConnectionQueue *queue);

/**
 * @brief Destroy mutexes, condition variables, and free allocated buffer.
 * @param queue Target queue.
 */
void connection_queue_destroy(ConnectionQueue *queue);

#endif /* CONNECTION_QUEUE_H */
