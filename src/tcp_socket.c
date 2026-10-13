/**
 * @file tcp_socket.c
 * @brief Raw POSIX TCP socket listener, option management, and robust stream I/O.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Intended Responsibilities:
 *  1. Socket Creation & Option Management:
 *     - Call socket(AF_INET, SOCK_STREAM, 0) directly.
 *     - Apply setsockopt() flags:
 *         * SO_REUSEADDR / SO_REUSEPORT: Prevent 'Address already in use' errors on restart.
 *         * SO_RCVTIMEO / SO_SNDTIMEO: Mitigate slow client attacks (e.g. Slowloris).
 *         * TCP_NODELAY: Disable Nagle algorithm where sub-millisecond header delivery is needed.
 *  2. Address Binding & Passive Listening:
 *     - Resolve IPv4/IPv6 addresses via getaddrinfo() or sockaddr_in.
 *     - Bind to designated interface and port.
 *     - Invoke listen() with specified kernel backlog.
 *  3. Connection Acceptance:
 *     - Accept incoming TCP handshakes via accept() / accept4().
 *     - Populate client IP string and ephemeral port for logging and telemetry.
 *  4. Robust TCP Stream I/O:
 *     - Ingest streaming byte chunks, accounting for TCP MTU fragmentation.
 *     - Handle partial writes in a loop until all payload bytes are flushed or an error occurs.
 *     - Distinguish clean client disconnections (EOF / read() returns 0) from socket errors.
 *  5. Descriptor Cleanup:
 *     - Safely close client sockets and mark handles as invalid (-1).
 *
 * Implementation Status:
 *  - Architecture and interface specification complete in include/tcp_socket.h.
 *  - POSIX networking implementation scheduled for development in Phase 1.
 */

#include "tcp_socket.h"
#include <unistd.h>

void tcp_close_socket(int *socket_fd) {
    if (socket_fd && *socket_fd >= 0) {
        close(*socket_fd);
        *socket_fd = -1;
    }
}
