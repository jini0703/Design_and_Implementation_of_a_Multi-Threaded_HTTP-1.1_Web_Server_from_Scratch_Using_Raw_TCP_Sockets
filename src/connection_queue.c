/**
 * @file connection_queue.c
 * @brief Thread-safe bounded connection queue implementation using POSIX synchronization.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include "connection_queue.h"
#include "util.h"
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

static void ms_to_timespec(int timeout_ms, struct timespec *ts) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    long sec = timeout_ms / 1000;
    long nsec = (timeout_ms % 1000) * 1000000L;
    ts->tv_sec = now.tv_sec + sec;
    ts->tv_nsec = now.tv_nsec + nsec;
    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_sec += 1;
        ts->tv_nsec -= 1000000000L;
    }
}

int connection_queue_init(ConnectionQueue *queue, size_t capacity) {
    if (!queue || capacity == 0) return -1;
    queue->buffer = (ClientConnection *)calloc(capacity, sizeof(ClientConnection));
    if (!queue->buffer) return -1;

    queue->capacity = capacity;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->is_shutdown = false;
    if (pthread_mutex_init(&queue->lock, NULL) != 0) {
        free(queue->buffer);
        queue->buffer = NULL;
        return -1;
    }
    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->lock);
        free(queue->buffer);
        queue->buffer = NULL;
        return -1;
    }
    if (pthread_cond_init(&queue->not_full, NULL) != 0) {
        pthread_cond_destroy(&queue->not_empty);
        pthread_mutex_destroy(&queue->lock);
        free(queue->buffer);
        queue->buffer = NULL;
        return -1;
    }
    return 0;
}

int connection_queue_push(ConnectionQueue *queue, const ClientConnection *conn, int timeout_ms) {
    if (!queue || !conn) return -1;

    pthread_mutex_lock(&queue->lock);

    while (queue->count == queue->capacity) {
        if (queue->is_shutdown) {
            pthread_mutex_unlock(&queue->lock);
            return -2;
        }
        if (timeout_ms == 0) {
            pthread_mutex_unlock(&queue->lock);
            return -1;
        }
        if (timeout_ms < 0) {
            pthread_cond_wait(&queue->not_full, &queue->lock);
        } else {
            struct timespec ts;
            ms_to_timespec(timeout_ms, &ts);
            int rc = pthread_cond_timedwait(&queue->not_full, &queue->lock, &ts);
            if (rc == ETIMEDOUT && queue->count == queue->capacity) {
                pthread_mutex_unlock(&queue->lock);
                return -1;
            }
        }
    }

    if (queue->is_shutdown) {
        pthread_mutex_unlock(&queue->lock);
        return -2;
    }

    queue->buffer[queue->tail] = *conn;
    if (queue->buffer[queue->tail].enqueued_ns == 0) {
        queue->buffer[queue->tail].enqueued_ns = monotonic_ns();
    }
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;

    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->lock);
    return 0;
}

int connection_queue_pop(ConnectionQueue *queue, ClientConnection *conn, int timeout_ms) {
    if (!queue || !conn) return -1;

    pthread_mutex_lock(&queue->lock);

    while (queue->count == 0) {
        if (queue->is_shutdown) {
            pthread_mutex_unlock(&queue->lock);
            return -2;
        }
        if (timeout_ms == 0) {
            pthread_mutex_unlock(&queue->lock);
            return -1;
        }
        if (timeout_ms < 0) {
            pthread_cond_wait(&queue->not_empty, &queue->lock);
        } else {
            struct timespec ts;
            ms_to_timespec(timeout_ms, &ts);
            int rc = pthread_cond_timedwait(&queue->not_empty, &queue->lock, &ts);
            if (rc == ETIMEDOUT && queue->count == 0) {
                pthread_mutex_unlock(&queue->lock);
                return -1;
            }
        }
    }

    if (queue->count == 0 && queue->is_shutdown) {
        pthread_mutex_unlock(&queue->lock);
        return -2;
    }

    *conn = queue->buffer[queue->head];
    conn->dequeued_ns = monotonic_ns();
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;

    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->lock);
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
    if (queue->buffer) {
        /* Drain any pending client sockets left unhandled */
        while (queue->count > 0) {
            ClientConnection *conn = &queue->buffer[queue->head];
            if (conn->fd > 2) {
                tcp_close(&conn->fd);
            }
            queue->head = (queue->head + 1) % queue->capacity;
            queue->count--;
        }
        free(queue->buffer);
        queue->buffer = NULL;
    }
    pthread_mutex_unlock(&queue->lock);

    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
}
