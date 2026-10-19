#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

/**
 * @file tcp_socket.h
 * @brief Raw POSIX TCP primitives: listener setup, accept, deadline-bounded I/O, close.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * This module knows nothing about HTTP. It provides:
 *  - tcp_listen_create(): socket() + SO_REUSEADDR + bind() + listen(), non-blocking.
 *  - tcp_accept_client(): accept() with errno classification; accepted sockets are
 *    made non-blocking, close-on-exec, and TCP_NODELAY.
 *  - tcp_recv_some()/tcp_send_all()/tcp_sendv_all()/tcp_sendfile_all(): I/O loops that
 *    tolerate EINTR, EAGAIN, partial reads, and partial writes, and that never wait
 *    past an absolute CLOCK_MONOTONIC deadline (implemented with poll()).
 *
 * SIGPIPE: sends use MSG_NOSIGNAL where the platform defines it (Linux). The server also
 * ignores SIGPIPE process-wide, which is the portable fallback for other systems.
 */

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/uio.h>

/* Negative return codes shared by the I/O helpers. */
#define TCP_ERR_IO       (-1) /**< Unrecoverable socket error (reset, EPIPE, ...). */
#define TCP_ERR_TIMEOUT  (-2) /**< The absolute deadline expired. */
#define TCP_ERR_STOPPED  (-3) /**< The optional stop flag was raised while waiting. */

/** Granularity at which waits re-check an optional stop flag. */
#define TCP_STOP_POLL_SLICE_MS 100

#define TCP_IP_STRLEN 64

/**
 * @brief An accepted client connection. Exactly one component owns `fd` at any time
 *        (acceptor -> queue -> worker); see docs/architecture.md, "Socket ownership".
 */
typedef struct {
    int fd;
    char ip[TCP_IP_STRLEN];
    uint16_t port;
    bool is_loopback;      /**< Peer address is 127.0.0.0/8, ::1, or ::ffff:127.0.0.0/104. */
    uint64_t accepted_ns;  /**< monotonic_ns() timestamp taken right after accept(). */
    uint64_t enqueued_ns;  /**< monotonic_ns() timestamp taken right before queue push. */
    uint64_t dequeued_ns;  /**< monotonic_ns() timestamp taken right after queue pop. */
} ClientConnection;

typedef enum {
    TCP_ACCEPT_OK = 0,
    TCP_ACCEPT_AGAIN,     /**< No pending connection (EAGAIN/EWOULDBLOCK). */
    TCP_ACCEPT_RETRY,     /**< Transient per-connection failure (EINTR, ECONNABORTED, ...). */
    TCP_ACCEPT_RESOURCE,  /**< Descriptor/memory exhaustion (EMFILE, ENFILE, ENOBUFS, ENOMEM). */
    TCP_ACCEPT_FATAL      /**< Unexpected error on the listening socket. */
} TcpAcceptResult;

/**
 * Create a non-blocking listening socket bound to host:port.
 * @param host Numeric IPv4/IPv6 address (e.g. "127.0.0.1", "0.0.0.0", "::1").
 * @param err  Receives a descriptive message on failure (may be NULL).
 * @return Listening descriptor (>= 0) or -1 on failure.
 */
int tcp_listen_create(const char *host, uint16_t port, int backlog, char *err, size_t errlen);

/** Accept one pending connection. On TCP_ACCEPT_OK, the caller owns out->fd. */
TcpAcceptResult tcp_accept_client(int listen_fd, ClientConnection *out, int *errno_out);

int tcp_set_nonblocking(int fd);
int tcp_set_cloexec(int fd);
int tcp_set_nodelay(int fd);

/**
 * Receive up to cap bytes, waiting no later than deadline_ns.
 * @param stop Optional flag; when it becomes true while waiting, returns TCP_ERR_STOPPED.
 * @return >0 bytes received, 0 on orderly EOF, or a negative TCP_ERR_* code.
 */
ssize_t tcp_recv_some(int fd, char *buf, size_t cap, uint64_t deadline_ns, const atomic_bool *stop);

/** Send all bytes (looping over partial writes). @return 0 or a negative TCP_ERR_* code. */
int tcp_send_all(int fd, const void *buf, size_t len, uint64_t deadline_ns, size_t *sent_out);

/** Gather-send up to 4 buffers completely. @return 0 or a negative TCP_ERR_* code. */
int tcp_sendv_all(int fd, const struct iovec *iov, int iovcnt, uint64_t deadline_ns, size_t *sent_out);

/**
 * Transmit `count` bytes of an open regular file starting at its offset 0.
 * Uses Linux sendfile(2) when use_sendfile is true and supported, otherwise a bounded
 * read()/send() loop. A file that shrinks during transfer yields TCP_ERR_IO, because the
 * already-sent Content-Length can no longer be honoured and the connection must close.
 */
int tcp_sendfile_all(int sock_fd, int file_fd, size_t count, uint64_t deadline_ns,
                     bool use_sendfile, size_t *sent_out);

/** Single non-blocking send attempt; result ignored (used for best-effort 503 rejections). */
void tcp_send_best_effort(int fd, const void *buf, size_t len);

/** Read and discard whatever is immediately available (bounded), without waiting. */
void tcp_discard_available(int fd, size_t max_bytes);

/** close() the descriptor if valid and set it to -1 (prevents double close). */
void tcp_close(int *fd);

/**
 * Half-close the write side, briefly drain unread input, then close. Used after error
 * responses so that unread request bytes do not trigger a TCP RST that could destroy the
 * response before the client reads it.
 */
void tcp_lingering_close(int *fd, int linger_ms);

#endif /* TCP_SOCKET_H */
