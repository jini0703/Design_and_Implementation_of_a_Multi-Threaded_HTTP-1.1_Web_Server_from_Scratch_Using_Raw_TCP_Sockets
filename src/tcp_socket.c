/**
 * @file tcp_socket.c
 * @brief Raw POSIX TCP socket operations with deadline-bounded polling.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#define _GNU_SOURCE
#include "tcp_socket.h"
#include "util.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__linux__)
#include <sys/sendfile.h>
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

int tcp_set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int tcp_set_cloexec(int fd)
{
    int flags = fcntl(fd, F_GETFD, 0);
    if (flags < 0) {
        return -1;
    }
    return fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
}

int tcp_set_nodelay(int fd)
{
    int one = 1;
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
}

static bool check_if_loopback(const struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        const struct sockaddr_in *sin = (const struct sockaddr_in *)sa;
        uint32_t addr = ntohl(sin->sin_addr.s_addr);
        return ((addr >> 24) == 127);
    } else if (sa->sa_family == AF_INET6) {
        const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)sa;
        const unsigned char *b = sin6->sin6_addr.s6_addr;
        /* ::1 check */
        bool is_v6_loopback = true;
        for (int i = 0; i < 15; i++) {
            if (b[i] != 0) {
                is_v6_loopback = false;
                break;
            }
        }
        if (is_v6_loopback && b[15] == 1) {
            return true;
        }
        /* IPv4-mapped ::ffff:127.x.x.x check */
        if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
            return (b[12] == 127);
        }
    }
    return false;
}

int tcp_listen_create(const char *host, uint16_t port, int backlog, char *err, size_t errlen)
{
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%u", port);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    struct addrinfo *res = NULL;
    int rc = getaddrinfo(host, port_str, &hints, &res);
    if (rc != 0) {
        if (err && errlen > 0) {
            snprintf(err, errlen, "getaddrinfo(\"%s\", %s): %s",
                     host ? host : "NULL", port_str, gai_strerror(rc));
        }
        return -1;
    }

    int listen_fd = -1;
    for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
        listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listen_fd < 0) {
            continue;
        }

        int one = 1;
        (void)setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

        if (p->ai_family == AF_INET6) {
            /* Keep IPv6 socket from binding IPv4 if host is explicitly specified */
            int v6only = (host != NULL && strcmp(host, "::") != 0) ? 1 : 0;
            (void)setsockopt(listen_fd, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only));
        }

        if (bind(listen_fd, p->ai_addr, p->ai_addrlen) == 0) {
            break; /* Successfully bound */
        }

        close(listen_fd);
        listen_fd = -1;
    }

    freeaddrinfo(res);

    if (listen_fd < 0) {
        if (err && errlen > 0) {
            snprintf(err, errlen, "Failed to bind to %s:%u: %s",
                     host ? host : "0.0.0.0", port, strerror(errno));
        }
        return -1;
    }

    if (listen(listen_fd, backlog > 0 ? backlog : SOMAXCONN) < 0) {
        if (err && errlen > 0) {
            snprintf(err, errlen, "listen() on port %u failed: %s", port, strerror(errno));
        }
        close(listen_fd);
        return -1;
    }

    (void)tcp_set_nonblocking(listen_fd);
    (void)tcp_set_cloexec(listen_fd);
    return listen_fd;
}

TcpAcceptResult tcp_accept_client(int listen_fd, ClientConnection *out, int *errno_out)
{
    if (!out) {
        return TCP_ACCEPT_FATAL;
    }

    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);

#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
    int cfd = accept4(listen_fd, (struct sockaddr *)&addr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
#else
    int cfd = accept(listen_fd, (struct sockaddr *)&addr, &addrlen);
#endif

    if (cfd < 0) {
        int e = errno;
        if (errno_out) *errno_out = e;
        if (e == EAGAIN || e == EWOULDBLOCK) {
            return TCP_ACCEPT_AGAIN;
        }
        if (e == EINTR || e == ECONNABORTED || e == EPROTO) {
            return TCP_ACCEPT_RETRY;
        }
        if (e == EMFILE || e == ENFILE || e == ENOBUFS || e == ENOMEM) {
            return TCP_ACCEPT_RESOURCE;
        }
        return TCP_ACCEPT_FATAL;
    }

#if !(defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC))
    (void)tcp_set_nonblocking(cfd);
    (void)tcp_set_cloexec(cfd);
#endif
    (void)tcp_set_nodelay(cfd);

    out->fd = cfd;
    out->accepted_ns = monotonic_ns();
    out->is_loopback = check_if_loopback((struct sockaddr *)&addr);

    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
    if (getnameinfo((struct sockaddr *)&addr, addrlen,
                    host, sizeof(host), serv, sizeof(serv),
                    NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
        strncpy(out->ip, host, sizeof(out->ip) - 1);
        out->ip[sizeof(out->ip) - 1] = '\0';
        out->port = (uint16_t)atoi(serv);
    } else {
        strncpy(out->ip, "unknown", sizeof(out->ip) - 1);
        out->ip[sizeof(out->ip) - 1] = '\0';
        out->port = 0;
    }

    return TCP_ACCEPT_OK;
}

static int wait_poll(int fd, short events, uint64_t deadline_ns, const atomic_bool *stop)
{
    while (1) {
        if (stop && atomic_load(stop)) {
            return TCP_ERR_STOPPED;
        }
        uint64_t now = monotonic_ns();
        if (deadline_ns != 0 && now >= deadline_ns) {
            return TCP_ERR_TIMEOUT;
        }

        int timeout_ms = -1;
        if (deadline_ns != 0) {
            uint64_t rem_ms = (deadline_ns - now) / NS_PER_MS;
            if (rem_ms > TCP_STOP_POLL_SLICE_MS && stop) {
                timeout_ms = TCP_STOP_POLL_SLICE_MS;
            } else {
                timeout_ms = (rem_ms > 0) ? (int)rem_ms : 0;
            }
        } else if (stop) {
            timeout_ms = TCP_STOP_POLL_SLICE_MS;
        }

        struct pollfd pfd = { .fd = fd, .events = events, .revents = 0 };
        int pr = poll(&pfd, 1, timeout_ms);
        if (pr < 0) {
            if (errno == EINTR) {
                continue;
            }
            return TCP_ERR_IO;
        }
        if (pr == 0) {
            /* Timeout on this slice; loop will check stop flag and overall deadline */
            continue;
        }
        if (pfd.revents & (POLLERR | POLLNVAL)) {
            return TCP_ERR_IO;
        }
        if (pfd.revents & (events | POLLHUP)) {
            return 0;
        }
    }
}

ssize_t tcp_recv_some(int fd, char *buf, size_t cap, uint64_t deadline_ns, const atomic_bool *stop)
{
    if (fd < 0 || !buf || cap == 0) {
        return TCP_ERR_IO;
    }

    while (1) {
        ssize_t n = recv(fd, buf, cap, 0);
        if (n > 0) {
            return n;
        }
        if (n == 0) {
            return 0; /* EOF */
        }
        int e = errno;
        if (e == EINTR) {
            continue;
        }
        if (e == EAGAIN || e == EWOULDBLOCK) {
            int w = wait_poll(fd, POLLIN, deadline_ns, stop);
            if (w < 0) {
                return w;
            }
            continue;
        }
        return TCP_ERR_IO;
    }
}

int tcp_send_all(int fd, const void *buf, size_t len, uint64_t deadline_ns, size_t *sent_out)
{
    if (sent_out) *sent_out = 0;
    if (fd < 0 || (!buf && len > 0)) {
        return TCP_ERR_IO;
    }

    const char *p = (const char *)buf;
    size_t remaining = len;

    while (remaining > 0) {
        ssize_t n = send(fd, p, remaining, MSG_NOSIGNAL);
        if (n > 0) {
            p += n;
            remaining -= (size_t)n;
            if (sent_out) *sent_out += (size_t)n;
            continue;
        }
        if (n < 0) {
            int e = errno;
            if (e == EINTR) {
                continue;
            }
            if (e == EAGAIN || e == EWOULDBLOCK) {
                int w = wait_poll(fd, POLLOUT, deadline_ns, NULL);
                if (w < 0) {
                    return w;
                }
                continue;
            }
            return TCP_ERR_IO;
        }
    }
    return 0;
}

int tcp_sendv_all(int fd, const struct iovec *iov, int iovcnt, uint64_t deadline_ns, size_t *sent_out)
{
    if (sent_out) *sent_out = 0;
    if (fd < 0 || !iov || iovcnt <= 0) {
        return TCP_ERR_IO;
    }

    struct iovec local_iov[4];
    if (iovcnt > 4) {
        return TCP_ERR_IO;
    }
    for (int i = 0; i < iovcnt; i++) {
        local_iov[i] = iov[i];
    }
    int cur = 0;

    while (cur < iovcnt) {
        ssize_t n = writev(fd, &local_iov[cur], iovcnt - cur);
        if (n > 0) {
            size_t written = (size_t)n;
            if (sent_out) *sent_out += written;
            while (cur < iovcnt && written >= local_iov[cur].iov_len) {
                written -= local_iov[cur].iov_len;
                cur++;
            }
            if (cur < iovcnt && written > 0) {
                local_iov[cur].iov_base = (char *)local_iov[cur].iov_base + written;
                local_iov[cur].iov_len -= written;
            }
            continue;
        }
        if (n < 0) {
            int e = errno;
            if (e == EINTR) {
                continue;
            }
            if (e == EAGAIN || e == EWOULDBLOCK) {
                int w = wait_poll(fd, POLLOUT, deadline_ns, NULL);
                if (w < 0) {
                    return w;
                }
                continue;
            }
            return TCP_ERR_IO;
        }
    }
    return 0;
}

int tcp_sendfile_all(int sock_fd, int file_fd, size_t count, uint64_t deadline_ns,
                     bool use_sendfile, size_t *sent_out)
{
    if (sent_out) *sent_out = 0;
    if (sock_fd < 0 || file_fd < 0) {
        return TCP_ERR_IO;
    }

    size_t remaining = count;
    off_t offset = lseek(file_fd, 0, SEEK_CUR);
    if (offset == (off_t)-1) {
        offset = 0;
    }

#if defined(__linux__)
    if (use_sendfile) {
        while (remaining > 0) {
            size_t chunk = remaining > (1024 * 1024) ? (1024 * 1024) : remaining;
            ssize_t n = sendfile(sock_fd, file_fd, &offset, chunk);
            if (n > 0) {
                remaining -= (size_t)n;
                if (sent_out) *sent_out += (size_t)n;
                continue;
            }
            if (n == 0) {
                /* File truncated beneath us */
                return TCP_ERR_IO;
            }
            int e = errno;
            if (e == EINTR) {
                continue;
            }
            if (e == EAGAIN || e == EWOULDBLOCK) {
                int w = wait_poll(sock_fd, POLLOUT, deadline_ns, NULL);
                if (w < 0) {
                    return w;
                }
                continue;
            }
            /* sendfile failed with e.g. EINVAL on certain filesystems; fall back to read loop */
            break;
        }
        if (remaining == 0) {
            return 0;
        }
    }
#else
    (void)use_sendfile;
#endif

    /* Userspace buffer fallback loop: synchronize file descriptor offset to position reached */
    if (lseek(file_fd, offset, SEEK_SET) == (off_t)-1) {
        return TCP_ERR_IO;
    }
    char buf[16384];
    while (remaining > 0) {
        size_t to_read = remaining < sizeof(buf) ? remaining : sizeof(buf);
        ssize_t r = read(file_fd, buf, to_read);
        if (r < 0) {
            if (errno == EINTR) continue;
            return TCP_ERR_IO;
        }
        if (r == 0) {
            return TCP_ERR_IO; /* Unexpected early EOF */
        }
        size_t written = 0;
        int rc = tcp_send_all(sock_fd, buf, (size_t)r, deadline_ns, &written);
        if (sent_out) *sent_out += written;
        if (rc < 0) {
            return rc;
        }
        remaining -= (size_t)r;
    }
    return 0;
}

void tcp_send_best_effort(int fd, const void *buf, size_t len)
{
    if (fd < 0 || !buf || len == 0) return;
    (void)send(fd, buf, len, MSG_NOSIGNAL | MSG_DONTWAIT);
}

void tcp_discard_available(int fd, size_t max_bytes)
{
    if (fd < 0) return;
    char buf[4096];
    size_t discarded = 0;
    while (discarded < max_bytes) {
        size_t to_read = (max_bytes - discarded < sizeof(buf)) ? (max_bytes - discarded) : sizeof(buf);
        ssize_t n = recv(fd, buf, to_read, MSG_DONTWAIT);
        if (n <= 0) break;
        discarded += (size_t)n;
    }
}

void tcp_close(int *fd)
{
    if (fd && *fd >= 0) {
        close(*fd);
        *fd = -1;
    }
}

void tcp_lingering_close(int *fd, int linger_ms)
{
    if (!fd || *fd < 0) return;
    (void)shutdown(*fd, SHUT_WR);
    if (linger_ms > 0) {
        struct pollfd pfd = { .fd = *fd, .events = POLLIN, .revents = 0 };
        if (poll(&pfd, 1, linger_ms) > 0 && (pfd.revents & POLLIN)) {
            tcp_discard_available(*fd, 65536);
        }
    }
    tcp_close(fd);
}
