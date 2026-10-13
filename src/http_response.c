/**
 * @file http_response.c
 * @brief HTTP/1.1 response formatting, status phrase mapping, and header serialization.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Intended Responsibilities:
 *  1. Status Code Mapping:
 *     - Translate numerical HTTP status codes (200, 400, 404, 500, etc.) to standard RFC reason phrases.
 *  2. Standard Header Construction:
 *     - Generate HTTP 'Date' header using RFC 1123 format (gmtime_r + strftime).
 *     - Add 'Server' identification token.
 *     - Add 'Content-Type' based on static file MIME resolution.
 *     - Add 'Content-Length' for deterministic response framing.
 *     - Add 'Connection: keep-alive' or 'Connection: close' matching connection policy.
 *  3. Error Response Templating:
 *     - Construct uniform, informative HTML and JSON error payloads for 4xx and 5xx errors.
 *  4. Wire Serialization:
 *     - Format headers into a single buffer ending with CRLF CRLF.
 *     - Transmit payload via direct buffer write or zero-copy sendfile(2).
 *
 * Implementation Status:
 *  - Architecture and interface specification complete in include/http_response.h.
 *  - Response formatting logic scheduled for development in Phase 2.
 */

#include "http_response.h"
#include <stdio.h>
#include <string.h>

const char *http_status_reason_phrase(HttpStatus status) {
    switch (status) {
        case HTTP_STATUS_OK:                  return "OK";
        case HTTP_STATUS_NO_CONTENT:          return "No Content";
        case HTTP_STATUS_MOVED_PERMANENTLY:   return "Moved Permanently";
        case HTTP_STATUS_NOT_MODIFIED:        return "Not Modified";
        case HTTP_STATUS_BAD_REQUEST:         return "Bad Request";
        case HTTP_STATUS_FORBIDDEN:           return "Forbidden";
        case HTTP_STATUS_NOT_FOUND:           return "Not Found";
        case HTTP_STATUS_METHOD_NOT_ALLOWED:  return "Method Not Allowed";
        case HTTP_STATUS_REQUEST_TIMEOUT:     return "Request Timeout";
        case HTTP_STATUS_PAYLOAD_TOO_LARGE:   return "Payload Too Large";
        case HTTP_STATUS_URI_TOO_LONG:        return "URI Too Long";
        case HTTP_STATUS_INTERNAL_ERROR:      return "Internal Server Error";
        case HTTP_STATUS_SERVICE_UNAVAILABLE: return "Service Unavailable";
        default:                              return "Unknown";
    }
}

void http_response_init(HttpResponse *res) {
    if (!res) return;
    memset(res, 0, sizeof(HttpResponse));
    res->status = HTTP_STATUS_OK;
    res->file_fd = -1;
    res->is_keep_alive = true;
}

void http_response_build_error(HttpResponse *res, HttpStatus status, const char *custom_message) {
    if (!res) return;
    http_response_init(res);
    res->status = status;
    res->is_keep_alive = false;
    /* In the full implementation, body will contain a formatted HTML or JSON error payload */
    (void)custom_message;
}

void http_response_destroy(HttpResponse *res) {
    if (!res) return;
    res->body = NULL;
    res->body_len = 0;
    res->file_fd = -1;
}
