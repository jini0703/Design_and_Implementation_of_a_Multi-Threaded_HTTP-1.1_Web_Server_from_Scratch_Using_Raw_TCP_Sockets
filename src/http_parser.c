/**
 * @file http_parser.c
 * @brief Zero-third-party RFC 9112 streaming HTTP/1.1 request parser.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include "http_parser.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

int conn_buffer_init(ConnectionBuffer *cb, size_t capacity)
{
    if (!cb || capacity == 0) return -1;
    cb->buf = (char *)malloc(capacity);
    if (!cb->buf) return -1;
    cb->capacity = capacity;
    cb->length = 0;
    return 0;
}

void conn_buffer_consume(ConnectionBuffer *cb, size_t n)
{
    if (!cb || n == 0) return;
    if (n >= cb->length) {
        cb->length = 0;
    } else {
        memmove(cb->buf, cb->buf + n, cb->length - n);
        cb->length -= n;
    }
}

void conn_buffer_free(ConnectionBuffer *cb)
{
    if (cb && cb->buf) {
        free(cb->buf);
        cb->buf = NULL;
        cb->capacity = 0;
        cb->length = 0;
    }
}

void http_request_init(HttpRequest *req)
{
    if (!req) return;
    memset(req, 0, sizeof(HttpRequest));
    req->method = HTTP_METHOD_UNKNOWN;
    req->content_length = -1;
    req->keep_alive = true;
}

void http_request_destroy(HttpRequest *req)
{
    if (!req) return;
    if (req->owns_body && req->body) {
        free(req->body);
        req->body = NULL;
    }
    req->owns_body = false;
}

const char *http_request_get_header(const HttpRequest *req, const char *name)
{
    if (!req || !name) return NULL;
    for (size_t i = 0; i < req->header_count; ++i) {
        if (strcasecmp(req->headers[i].name, name) == 0) {
            return req->headers[i].value;
        }
    }
    return NULL;
}

int parser_status_to_http_code(ParserStatus status)
{
    switch (status) {
        case PARSER_OK:                        return 200;
        case PARSER_NEED_MORE_DATA:            return 0;
        case PARSER_ERR_BAD_REQUEST:           return 400;
        case PARSER_ERR_METHOD_NOT_ALLOWED:    return 405;
        case PARSER_ERR_NOT_IMPLEMENTED:       return 501;
        case PARSER_ERR_URI_TOO_LONG:          return 414;
        case PARSER_ERR_HEADERS_TOO_LARGE:     return 431;
        case PARSER_ERR_BODY_TOO_LARGE:        return 413;
        case PARSER_ERR_VERSION_NOT_SUPPORTED: return 505;
        case PARSER_ERR_EXPECTATION_FAILED:    return 417;
        case PARSER_ERR_LENGTH_REQUIRED:       return 411;
        default:                               return 400;
    }
}

static const char *find_crlf_crlf(const char *data, size_t len)
{
    if (len < 4) return NULL;
    for (size_t i = 0; i + 3 < len; i++) {
        if (data[i] == '\r' && data[i+1] == '\n' && data[i+2] == '\r' && data[i+3] == '\n') {
            return data + i;
        }
    }
    return NULL;
}

static char *trim_whitespace(char *str)
{
    while (*str == ' ' || *str == '\t') str++;
    if (*str == '\0') return str;
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }
    return str;
}

ParserStatus http_parse_request(const char *raw_data, size_t length,
                               size_t max_header_size, size_t max_body_size,
                               HttpRequest *req, size_t *bytes_consumed)
{
    if (bytes_consumed) *bytes_consumed = 0;
    if (!raw_data || length == 0 || !req) {
        return PARSER_NEED_MORE_DATA;
    }

    /* 1. Locate end of headers (\r\n\r\n) */
    const char *hdr_end = find_crlf_crlf(raw_data, length);
    if (!hdr_end) {
        /* Reject bare LF line feeds (\n\n) per RFC 9112 */
        for (size_t i = 0; i + 1 < length; i++) {
            if (raw_data[i] == '\n' && raw_data[i+1] == '\n') {
                return PARSER_ERR_BAD_REQUEST;
            }
        }
        if (length > max_header_size) {
            return PARSER_ERR_HEADERS_TOO_LARGE;
        }
        return PARSER_NEED_MORE_DATA;
    }

    size_t header_block_len = (size_t)(hdr_end - raw_data) + 4;
    if (header_block_len > max_header_size) {
        return PARSER_ERR_HEADERS_TOO_LARGE;
    }

    http_request_init(req);

    /* Copy header block into temporary buffer for safe tokenization */
    char *hdr_buf = (char *)malloc(header_block_len + 1);
    if (!hdr_buf) {
        return PARSER_ERR_BAD_REQUEST;
    }
    memcpy(hdr_buf, raw_data, header_block_len);
    hdr_buf[header_block_len] = '\0';

    /* 2. Parse Request Line: Method SP Request-URI SP HTTP-Version CRLF */
    char *line_end = strstr(hdr_buf, "\r\n");
    if (!line_end) {
        free(hdr_buf);
        return PARSER_ERR_BAD_REQUEST;
    }
    *line_end = '\0';
    char *req_line = hdr_buf;

    /* Extract Method */
    char *sp1 = strchr(req_line, ' ');
    if (!sp1 || sp1 == req_line) {
        free(hdr_buf);
        return PARSER_ERR_BAD_REQUEST;
    }
    *sp1 = '\0';
    const char *method_token = req_line;

    /* Extract Request-URI */
    char *uri_start = sp1 + 1;
    if (*uri_start == ' ') {
        /* RFC 9112 prohibits multiple whitespace characters between tokens */
        free(hdr_buf);
        return PARSER_ERR_BAD_REQUEST;
    }
    char *sp2 = strchr(uri_start, ' ');
    if (!sp2 || sp2 == uri_start) {
        free(hdr_buf);
        return PARSER_ERR_BAD_REQUEST;
    }
    *sp2 = '\0';
    const char *uri_token = uri_start;

    /* Extract HTTP-Version */
    char *version_token = sp2 + 1;
    if (*version_token == ' ' || *version_token == '\0') {
        free(hdr_buf);
        return PARSER_ERR_BAD_REQUEST;
    }

    /* Validate & assign Method */
    snprintf(req->method_raw, sizeof(req->method_raw), "%s", method_token);
    if (strcmp(method_token, "GET") == 0) {
        req->method = HTTP_METHOD_GET;
    } else if (strcmp(method_token, "HEAD") == 0) {
        req->method = HTTP_METHOD_HEAD;
    } else if (strcmp(method_token, "POST") == 0) {
        req->method = HTTP_METHOD_POST;
    } else if (strcmp(method_token, "PUT") == 0) {
        req->method = HTTP_METHOD_PUT;
    } else if (strcmp(method_token, "DELETE") == 0) {
        req->method = HTTP_METHOD_DELETE;
    } else if (strcmp(method_token, "OPTIONS") == 0) {
        req->method = HTTP_METHOD_OPTIONS;
    } else if (strcmp(method_token, "PATCH") == 0) {
        req->method = HTTP_METHOD_PATCH;
    } else {
        /* Check if method contains invalid characters */
        for (const char *p = method_token; *p; p++) {
            if (!isalpha((unsigned char)*p)) {
                free(hdr_buf);
                return PARSER_ERR_BAD_REQUEST;
            }
        }
        req->method = HTTP_METHOD_UNKNOWN;
    }

    /* Validate & assign URI */
    size_t uri_len = strlen(uri_token);
    if (uri_len >= HTTP_MAX_URI_LENGTH) {
        free(hdr_buf);
        return PARSER_ERR_URI_TOO_LONG;
    }
    if (uri_token[0] != '/' && strcmp(uri_token, "*") != 0 && strncmp(uri_token, "http://", 7) != 0) {
        free(hdr_buf);
        return PARSER_ERR_BAD_REQUEST;
    }
    snprintf(req->uri, sizeof(req->uri), "%s", uri_token);

    /* Split URI into path and query string */
    const char *target = uri_token;
    if (strncmp(target, "http://", 7) == 0) {
        const char *slash = strchr(target + 7, '/');
        const char *q = strchr(target + 7, '?');
        if (slash && (!q || slash < q)) {
            target = slash;
        } else if (q) {
            snprintf(req->path, sizeof(req->path), "/");
            snprintf(req->query, sizeof(req->query), "%s", q + 1);
            target = NULL;
        } else {
            snprintf(req->path, sizeof(req->path), "/");
            req->query[0] = '\0';
            target = NULL;
        }
    }

    if (target) {
        const char *qmark = strchr(target, '?');
        if (qmark) {
            size_t path_len = (size_t)(qmark - target);
            if (path_len >= sizeof(req->path)) path_len = sizeof(req->path) - 1;
            memcpy(req->path, target, path_len);
            req->path[path_len] = '\0';
            snprintf(req->query, sizeof(req->query), "%s", qmark + 1);
        } else {
            snprintf(req->path, sizeof(req->path), "%s", target);
            req->query[0] = '\0';
        }
    }

    /* Validate & assign Version */
    snprintf(req->http_version, sizeof(req->http_version), "%s", version_token);
    if (strcmp(version_token, "HTTP/1.1") == 0) {
        req->is_http_11 = true;
        req->is_http_10 = false;
        req->keep_alive = true; /* HTTP/1.1 defaults to keep-alive */
    } else if (strcmp(version_token, "HTTP/1.0") == 0) {
        req->is_http_11 = false;
        req->is_http_10 = true;
        req->keep_alive = false; /* HTTP/1.0 defaults to close */
    } else if (strncmp(version_token, "HTTP/", 5) == 0) {
        free(hdr_buf);
        return PARSER_ERR_VERSION_NOT_SUPPORTED;
    } else {
        free(hdr_buf);
        return PARSER_ERR_BAD_REQUEST;
    }

    /* 3. Parse Headers */
    char *cursor = line_end + 2; /* Advance past \r\n */
    bool host_found = false;
    bool cl_found = false;

    while (*cursor != '\0') {
        char *next_crlf = strstr(cursor, "\r\n");
        if (!next_crlf) break;
        if (next_crlf == cursor) {
            /* Empty line indicates end of header block */
            break;
        }
        *next_crlf = '\0';
        char *line = cursor;
        cursor = next_crlf + 2;

        /* Reject bare CR or LF characters within the header line per RFC 9112 §5.2 */
        if (strpbrk(line, "\r\n") != NULL) {
            free(hdr_buf);
            return PARSER_ERR_BAD_REQUEST;
        }

        /* Check for whitespace before colon (prohibited by RFC 9112 §5.1) */
        char *colon = strchr(line, ':');
        if (!colon || colon == line) {
            free(hdr_buf);
            return PARSER_ERR_BAD_REQUEST;
        }
        if (colon > line && (*(colon - 1) == ' ' || *(colon - 1) == '\t')) {
            free(hdr_buf);
            return PARSER_ERR_BAD_REQUEST;
        }

        *colon = '\0';
        char *hname = trim_whitespace(line);
        char *hval = trim_whitespace(colon + 1);

        /* Validate header field-name characters */
        for (const char *p = hname; *p; p++) {
            if (*p <= 32 || *p >= 127 || *p == '(' || *p == ')' || *p == '<' || *p == '>' ||
                *p == '@' || *p == ',' || *p == ';' || *p == '\\' || *p == '"' || *p == '/' ||
                *p == '[' || *p == ']' || *p == '?' || *p == '=' || *p == '{' || *p == '}') {
                free(hdr_buf);
                return PARSER_ERR_BAD_REQUEST;
            }
        }

        /* Reject individual headers exceeding designated buffer limits per RFC 9112 §5.1 */
        if (strlen(hname) >= HTTP_MAX_HEADER_NAME || strlen(hval) >= HTTP_MAX_HEADER_VALUE) {
            free(hdr_buf);
            return PARSER_ERR_HEADERS_TOO_LARGE;
        }

        if (req->header_count >= HTTP_MAX_HEADERS) {
            free(hdr_buf);
            return PARSER_ERR_HEADERS_TOO_LARGE;
        }

        HttpHeader *hdr = &req->headers[req->header_count++];
        snprintf(hdr->name, sizeof(hdr->name), "%s", hname);
        snprintf(hdr->value, sizeof(hdr->value), "%s", hval);

        /* Specific header semantics */
        if (strcasecmp(hname, "Host") == 0) {
            if (host_found) {
                /* Multiple Host headers prohibited by RFC 9112 §7.1.2 */
                free(hdr_buf);
                return PARSER_ERR_BAD_REQUEST;
            }
            host_found = true;
            snprintf(req->host, sizeof(req->host), "%s", hval);
            if (req->host[0] == '\0') {
                free(hdr_buf);
                return PARSER_ERR_BAD_REQUEST;
            }
        } else if (strcasecmp(hname, "Content-Length") == 0) {
            if (cl_found) {
                /* Conflicting/multiple Content-Length headers prohibited */
                free(hdr_buf);
                return PARSER_ERR_BAD_REQUEST;
            }
            cl_found = true;
            req->has_content_length = true;

            if (hval[0] == '\0') {
                free(hdr_buf);
                return PARSER_ERR_BAD_REQUEST;
            }

            /* Every character must conform strictly to RFC 9110 §8.6 1*DIGIT grammar */
            for (const char *p = hval; *p != '\0'; p++) {
                if (!isdigit((unsigned char)*p)) {
                    free(hdr_buf);
                    return PARSER_ERR_BAD_REQUEST;
                }
            }

            char *endptr = NULL;
            errno = 0;
            unsigned long long val = strtoull(hval, &endptr, 10);
            if (errno == ERANGE || val > (unsigned long long)max_body_size) {
                free(hdr_buf);
                return PARSER_ERR_BODY_TOO_LARGE;
            }
            if (endptr == hval || *endptr != '\0') {
                free(hdr_buf);
                return PARSER_ERR_BAD_REQUEST;
            }
            req->content_length = (int64_t)val;
        } else if (strcasecmp(hname, "Transfer-Encoding") == 0) {
            req->has_transfer_encoding = true;
        } else if (strcasecmp(hname, "Connection") == 0) {
            if (strcasestr(hval, "close") != NULL) {
                req->keep_alive = false;
            } else if (strcasestr(hval, "keep-alive") != NULL) {
                req->keep_alive = true;
            }
        } else if (strcasecmp(hname, "Expect") == 0) {
            if (strcasestr(hval, "100-continue") != NULL) {
                req->has_expect_100 = true;
            }
        }
    }

    free(hdr_buf);

    /* 4. Post-Header Invariants */

    /* Host header is mandatory in HTTP/1.1 */
    if (req->is_http_11 && !host_found) {
        return PARSER_ERR_BAD_REQUEST;
    }

    /* Conflicting framing: Transfer-Encoding + Content-Length rejected */
    if (req->has_transfer_encoding && req->has_content_length) {
        return PARSER_ERR_BAD_REQUEST;
    }

    /* Unsupported Transfer-Encoding -> 501 Not Implemented */
    if (req->has_transfer_encoding) {
        return PARSER_ERR_NOT_IMPLEMENTED;
    }

    /* Expect: 100-continue -> 417 Expectation Failed */
    if (req->has_expect_100) {
        return PARSER_ERR_EXPECTATION_FAILED;
    }

    /* Method check */
    if (req->method == HTTP_METHOD_UNKNOWN) {
        return PARSER_ERR_NOT_IMPLEMENTED;
    }
    if (req->method != HTTP_METHOD_GET && req->method != HTTP_METHOD_HEAD && req->method != HTTP_METHOD_POST) {
        return PARSER_ERR_METHOD_NOT_ALLOWED;
    }

    /* POST requires Content-Length in our bounded server */
    if (req->method == HTTP_METHOD_POST && !req->has_content_length) {
        return PARSER_ERR_LENGTH_REQUIRED;
    }

    /* 5. Message Body Ingestion */
    size_t body_bytes_needed = (req->content_length > 0) ? (size_t)req->content_length : 0;
    size_t total_req_len = header_block_len + body_bytes_needed;

    if (length < total_req_len) {
        return PARSER_NEED_MORE_DATA;
    }

    if (body_bytes_needed > 0) {
        /* Allocate isolated copy of the body so it remains valid after buffer consumption */
        req->body = (char *)malloc(body_bytes_needed + 1);
        if (!req->body) {
            return PARSER_ERR_BAD_REQUEST;
        }
        memcpy(req->body, raw_data + header_block_len, body_bytes_needed);
        req->body[body_bytes_needed] = '\0';
        req->body_len = body_bytes_needed;
        req->owns_body = true;
    } else {
        req->body = NULL;
        req->body_len = 0;
        req->owns_body = false;
    }

    req->total_request_bytes = total_req_len;
    if (bytes_consumed) {
        *bytes_consumed = total_req_len;
    }

    return PARSER_OK;
}
