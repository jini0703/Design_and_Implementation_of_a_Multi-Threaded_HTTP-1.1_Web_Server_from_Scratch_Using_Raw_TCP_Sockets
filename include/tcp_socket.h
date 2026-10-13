#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

/**
 * @file tcp_socket.h
 * @brief POSIX TCP socket initialization, option configuration, and I/O primitives.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Module Responsibilities:
 *  - Initialize listening TCP stream sockets using raw POSIX socket APIs.
 *  - Configure essential socket options (SO_REUSEADDR, SO_RCVTIMEO, SO_SNDTIMEO, TCP_NODELAY).
 *  - Perform host address resolution, bind(2), and listen(2) with defined backlog.
 *  - Accept incoming connections with client IP/port metadata extraction.
 *  - Provide robust, bounded read and write utilities accounting for TCP stream fragmentation,
 *    EAGAIN/EWOULDBLOCK, partial reads, and partial writes.
 *  - Provide safe, centralized socket closure and descriptor invalidation.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

/**
 * @brief Representation of an accepted client connection.
 */
typedef struct {
    int client_fd;
    char client_ip[64];
    uint16_t client_port;
} ClientConnection;

/**
 * @brief Create, bind, and configure a passive listening TCP server socket.
 * @param host IP address string (e.g. "0.0.0.0" or "127.0.0.1").
 * @param port TCP port number.
 * @param backlog Maximum length to which the queue of pending connections may grow.
 * @return Listening socket file descriptor, or negative integer on failure.
 */
int tcp_listen_create(const char *host, uint16_t port, int backlog);

/**
 * @brief Configure send/receive timeouts on a socket descriptor.
 * @param socket_fd Target socket file descriptor.
 * @param timeout_sec Timeout duration in seconds.
 * @return 0 on success, -1 on failure.
 */
int tcp_set_timeouts(int socket_fd, int timeout_sec);

/**
 * @brief Enable or disable TCP_NODELAY (Nagle's algorithm) on a socket descriptor.
 * @param socket_fd Target socket file descriptor.
 * @param enable True to disable Nagle (low latency), False to retain default buffering.
 * @return 0 on success, -1 on failure.
 */
int tcp_set_nodelay(int socket_fd, bool enable);

/**
 * @brief Accept an incoming client connection from a listening socket.
 * @param listen_fd Server listening socket file descriptor.
 * @param conn Pointer to ClientConnection struct to receive descriptor and address info.
 * @return 0 on successful accept, negative error code on failure.
 */
int tcp_accept_client(int listen_fd, ClientConnection *conn);

/**
 * @brief Read bytes from a socket into a buffer, handling partial TCP reads and timeouts.
 * @param socket_fd Client socket descriptor.
 * @param buffer Destination buffer.
 * @param max_len Maximum bytes to read into buffer.
 * @return Number of bytes read (>0), 0 on orderly client disconnect (EOF), or -1 on error.
 */
ssize_t tcp_read_stream(int socket_fd, char *buffer, size_t max_len);

/**
 * @brief Write all bytes in a buffer to a socket, looping to handle partial TCP writes.
 * @param socket_fd Client socket descriptor.
 * @param buffer Data to transmit.
 * @param length Total length of data to write.
 * @return Number of bytes successfully sent, or -1 on unrecoverable transmission error.
 */
ssize_t tcp_write_all(int socket_fd, const char *buffer, size_t length);

/**
 * @brief Safely close a socket descriptor and set descriptor handle to -1.
 * @param socket_fd Pointer to socket file descriptor.
 */
void tcp_close_socket(int *socket_fd);

#endif /* TCP_SOCKET_H */
