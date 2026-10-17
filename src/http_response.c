/**
 * @file http_response.c
 * @brief HTTP/1.1 response formatting, status phrase mapping, and socket transmission.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include "http_response.h"
#include "tcp_socket.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

const char *http_status_reason_phrase(HttpStatus status)
{
    switch (status) {
        case HTTP_STATUS_OK:                    return "OK";
        case HTTP_STATUS_CREATED:               return "Created";
        case HTTP_STATUS_NO_CONTENT:            return "No Content";
        case HTTP_STATUS_MOVED_PERMANENTLY:     return "Moved Permanently";
        case HTTP_STATUS_NOT_MODIFIED:          return "Not Modified";
        case HTTP_STATUS_BAD_REQUEST:           return "Bad Request";
        case HTTP_STATUS_FORBIDDEN:             return "Forbidden";
        case HTTP_STATUS_NOT_FOUND:             return "Not Found";
        case HTTP_STATUS_METHOD_NOT_ALLOWED:    return "Method Not Allowed";
        case HTTP_STATUS_REQUEST_TIMEOUT:       return "Request Timeout";
        case HTTP_STATUS_LENGTH_REQUIRED:       return "Length Required";
        case HTTP_STATUS_PAYLOAD_TOO_LARGE:     return "Payload Too Large";
        case HTTP_STATUS_URI_TOO_LONG:          return "URI Too Long";
        case HTTP_STATUS_EXPECTATION_FAILED:    return "Expectation Failed";
        case HTTP_STATUS_HEADERS_TOO_LARGE:     return "Request Header Fields Too Large";
        case HTTP_STATUS_INTERNAL_ERROR:        return "Internal Server Error";
        case HTTP_STATUS_NOT_IMPLEMENTED:       return "Not Implemented";
        case HTTP_STATUS_SERVICE_UNAVAILABLE:   return "Service Unavailable";
        case HTTP_STATUS_VERSION_NOT_SUPPORTED: return "HTTP Version Not Supported";
        default:                                return "Unknown";
    }
}

void http_response_init(HttpResponse *res)
{
    if (!res) return;
    memset(res, 0, sizeof(HttpResponse));
    res->status = HTTP_STATUS_OK;
    res->file_fd = -1;
    res->is_keep_alive = true;
}

void http_response_set_status(HttpResponse *res, HttpStatus status)
{
    if (res) {
        res->status = status;
    }
}

int http_response_add_header(HttpResponse *res, const char *name, const char *value)
{
    if (!res || !name || !value) return -1;

    /* Check if header already exists, replace value if so */
    for (size_t i = 0; i < res->header_count; ++i) {
        if (strcasecmp(res->headers[i].name, name) == 0) {
            snprintf(res->headers[i].value, sizeof(res->headers[i].value), "%s", value);
            return 0;
        }
    }

    if (res->header_count >= HTTP_MAX_HEADERS) {
        return -1;
    }

    HttpHeader *hdr = &res->headers[res->header_count++];
    snprintf(hdr->name, sizeof(hdr->name), "%s", name);
    snprintf(hdr->value, sizeof(hdr->value), "%s", value);
    return 0;
}

int http_response_set_body_copy(HttpResponse *res, const char *content_type, const void *data, size_t len)
{
    if (!res) return -1;
    if (res->owns_body && res->body) {
        free(res->body);
        res->body = NULL;
    }

    if (len > 0 && data != NULL) {
        res->body = (char *)malloc(len + 1);
        if (!res->body) return -1;
        memcpy(res->body, data, len);
        res->body[len] = '\0';
        res->body_len = len;
        res->owns_body = true;
    } else {
        res->body = NULL;
        res->body_len = 0;
        res->owns_body = false;
    }

    if (content_type) {
        http_response_add_header(res, "Content-Type", content_type);
    }
    char cl_buf[32];
    snprintf(cl_buf, sizeof(cl_buf), "%zu", len);
    http_response_add_header(res, "Content-Length", cl_buf);
    return 0;
}

int http_response_set_body_nocopy(HttpResponse *res, const char *content_type, void *data, size_t len, bool take_ownership)
{
    if (!res) return -1;
    if (res->owns_body && res->body) {
        free(res->body);
    }
    res->body = (char *)data;
    res->body_len = len;
    res->owns_body = take_ownership;

    if (content_type) {
        http_response_add_header(res, "Content-Type", content_type);
    }
    char cl_buf[32];
    snprintf(cl_buf, sizeof(cl_buf), "%zu", len);
    http_response_add_header(res, "Content-Length", cl_buf);
    return 0;
}

void http_response_set_file(HttpResponse *res, const char *content_type, int file_fd, size_t file_size)
{
    if (!res) return;
    if (res->owns_body && res->body) {
        free(res->body);
        res->body = NULL;
        res->body_len = 0;
        res->owns_body = false;
    }
    res->file_fd = file_fd;
    res->file_size = file_size;

    if (content_type) {
        http_response_add_header(res, "Content-Type", content_type);
    }
    char cl_buf[32];
    snprintf(cl_buf, sizeof(cl_buf), "%zu", file_size);
    http_response_add_header(res, "Content-Length", cl_buf);
}

void http_response_build_error(HttpResponse *res, HttpStatus status, const char *custom_message, bool keep_alive)
{
    if (!res) return;
    http_response_init(res);
    res->status = status;
    res->is_keep_alive = keep_alive;

    const char *reason = http_status_reason_phrase(status);
    const char *msg = custom_message ? custom_message : reason;

    char html[1024];
    int n = snprintf(html, sizeof(html),
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head><meta charset=\"UTF-8\"><title>%d %s</title></head>\n"
        "<body style=\"font-family: system-ui, sans-serif; background: #0f172a; color: #f8fafc; padding: 3rem; text-align: center;\">\n"
        "  <h1 style=\"font-size: 2.5rem; margin-bottom: 0.5rem;\">%d %s</h1>\n"
        "  <p style=\"color: #94a3b8; font-size: 1.1rem;\">%s</p>\n"
        "  <hr style=\"border: 0; border-top: 1px solid #334155; margin: 2rem auto; max-width: 500px;\">\n"
        "  <small style=\"color: #64748b;\">HTTP/1.1 C17 POSIX Web Server</small>\n"
        "</body>\n"
        "</html>\n",
        (int)status, reason, (int)status, reason, msg);

    if (n > 0) {
        http_response_set_body_copy(res, "text/html; charset=utf-8", html, (size_t)n);
    }

    if (status == HTTP_STATUS_SERVICE_UNAVAILABLE) {
        http_response_add_header(res, "Retry-After", "5");
    }

    if (!keep_alive) {
        http_response_add_header(res, "Connection", "close");
    }
}

static void get_rfc1123_date(char *buf, size_t cap)
{
    time_t now = time(NULL);
    struct tm tm_utc;
    if (gmtime_r(&now, &tm_utc)) {
        strftime(buf, cap, "%a, %d %b %Y %H:%M:%S GMT", &tm_utc);
    } else {
        snprintf(buf, cap, "Thu, 01 Jan 1970 00:00:00 GMT");
    }
}

ssize_t http_response_serialize_headers(const HttpResponse *res, char *buffer, size_t buffer_size)
{
    if (!res || !buffer || buffer_size == 0) return -1;

    const char *reason = http_status_reason_phrase(res->status);
    int written = snprintf(buffer, buffer_size, "HTTP/1.1 %d %s\r\n", (int)res->status, reason);
    if (written < 0 || (size_t)written >= buffer_size) {
        return -1;
    }
    size_t offset = (size_t)written;

    /* Add Date header if not explicitly present */
    bool has_date = false;
    bool has_server = false;
    bool has_conn = false;

    for (size_t i = 0; i < res->header_count; ++i) {
        if (strcasecmp(res->headers[i].name, "Date") == 0) has_date = true;
        if (strcasecmp(res->headers[i].name, "Server") == 0) has_server = true;
        if (strcasecmp(res->headers[i].name, "Connection") == 0) has_conn = true;
    }

    if (!has_date) {
        char date_buf[64];
        get_rfc1123_date(date_buf, sizeof(date_buf));
        int n = snprintf(buffer + offset, buffer_size - offset, "Date: %s\r\n", date_buf);
        if (n < 0 || (size_t)n >= buffer_size - offset) return -1;
        offset += (size_t)n;
    }

    if (!has_server) {
        int n = snprintf(buffer + offset, buffer_size - offset, "Server: C17-POSIX-HTTP-Server/1.0\r\n");
        if (n < 0 || (size_t)n >= buffer_size - offset) return -1;
        offset += (size_t)n;
    }

    if (!has_conn) {
        const char *conn_val = res->is_keep_alive ? "keep-alive" : "close";
        int n = snprintf(buffer + offset, buffer_size - offset, "Connection: %s\r\n", conn_val);
        if (n < 0 || (size_t)n >= buffer_size - offset) return -1;
        offset += (size_t)n;
    }

    /* Serialize explicitly configured headers */
    for (size_t i = 0; i < res->header_count; ++i) {
        int n = snprintf(buffer + offset, buffer_size - offset, "%s: %s\r\n",
                         res->headers[i].name, res->headers[i].value);
        if (n < 0 || (size_t)n >= buffer_size - offset) return -1;
        offset += (size_t)n;
    }

    /* Final CRLF marking end of headers */
    if (offset + 2 >= buffer_size) return -1;
    buffer[offset++] = '\r';
    buffer[offset++] = '\n';
    buffer[offset] = '\0';

    return (ssize_t)offset;
}

int http_response_send(int client_fd, HttpResponse *res, uint64_t deadline_ns,
                       bool use_sendfile, size_t *bytes_sent)
{
    if (bytes_sent) *bytes_sent = 0;
    if (client_fd < 0 || !res) return TCP_ERR_IO;

    char header_buf[4096];
    ssize_t hlen = http_response_serialize_headers(res, header_buf, sizeof(header_buf));
    if (hlen < 0) {
        return TCP_ERR_IO;
    }

    /* HEAD requests omit response message body entirely */
    if (res->is_head_only) {
        size_t sent = 0;
        int rc = tcp_send_all(client_fd, header_buf, (size_t)hlen, deadline_ns, &sent);
        if (bytes_sent) *bytes_sent = sent;
        return rc;
    }

    /* File delivery (static asset) */
    if (res->file_fd >= 0) {
        size_t hsent = 0;
        int rc = tcp_send_all(client_fd, header_buf, (size_t)hlen, deadline_ns, &hsent);
        if (bytes_sent) *bytes_sent += hsent;
        if (rc < 0) return rc;

        if (res->file_size > 0) {
            size_t fsent = 0;
            rc = tcp_sendfile_all(client_fd, res->file_fd, res->file_size, deadline_ns, use_sendfile, &fsent);
            if (bytes_sent) *bytes_sent += fsent;
            return rc;
        }
        return 0;
    }

    /* In-memory body delivery */
    if (res->body && res->body_len > 0) {
        struct iovec iov[2];
        iov[0].iov_base = header_buf;
        iov[0].iov_len = (size_t)hlen;
        iov[1].iov_base = res->body;
        iov[1].iov_len = res->body_len;

        size_t total_sent = 0;
        int rc = tcp_sendv_all(client_fd, iov, 2, deadline_ns, &total_sent);
        if (bytes_sent) *bytes_sent = total_sent;
        return rc;
    }

    /* Header-only response (e.g. 204 No Content, 304 Not Modified) */
    size_t sent = 0;
    int rc = tcp_send_all(client_fd, header_buf, (size_t)hlen, deadline_ns, &sent);
    if (bytes_sent) *bytes_sent = sent;
    return rc;
}

void http_response_destroy(HttpResponse *res)
{
    if (!res) return;
    if (res->owns_body && res->body) {
        free(res->body);
        res->body = NULL;
    }
    if (res->file_fd >= 0) {
        close(res->file_fd);
        res->file_fd = -1;
    }
    res->body_len = 0;
    res->owns_body = false;
}
