/**
 * @file http_parser.c
 * @brief RFC 9112 compliant streaming HTTP/1.1 request parser implemented from scratch.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Intended Responsibilities:
 *  1. Zero-Dependency Protocol Parsing:
 *     - Parse HTTP requests directly from raw TCP byte streams without external libraries.
 *  2. Request-Line Processing:
 *     - Parse 'Method SP Request-URI SP HTTP-Version CRLF'.
 *     - Validate supported methods (GET, HEAD, POST).
 *     - Verify HTTP-Version is HTTP/1.1 (or HTTP/1.0 fallback).
 *  3. Header Processing & Field Extraction:
 *     - Extract header lines delimited by CRLF.
 *     - Strip optional whitespace (OWS) around header values.
 *     - Implement case-insensitive comparison (strcasecmp) for header lookups.
 *     - Enforce maximum header count (HTTP_MAX_HEADERS) and cumulative size limits.
 *  4. Framing & Body Ingestion:
 *     - Parse 'Content-Length' header for body sizing.
 *     - Reject conflicting or ambiguous transfer codings.
 *     - Read and buffer message bodies within strict memory limits.
 *  5. Connection Lifecycle Semantics:
 *     - Interpret 'Connection: keep-alive' vs 'Connection: close'.
 *     - Default to persistent connection for HTTP/1.1 unless 'close' is specified.
 *
 * Implementation Status:
 *  - Architecture and interface specification complete in include/http_parser.h.
 *  - Streaming state machine implementation scheduled for development in Phase 2.
 */

#include "http_parser.h"
#include <string.h>
#include <strings.h>

void http_request_init(HttpRequest *req) {
    if (!req) return;
    memset(req, 0, sizeof(HttpRequest));
    req->method = HTTP_METHOD_UNKNOWN;
    req->keep_alive = true; /* HTTP/1.1 defaults to keep-alive */
    req->content_length = -1;
    req->body = NULL;
}

const char *http_request_get_header(const HttpRequest *req, const char *name) {
    if (!req || !name) return NULL;
    for (size_t i = 0; i < req->header_count; ++i) {
        if (strcasecmp(req->headers[i].name, name) == 0) {
            return req->headers[i].value;
        }
    }
    return NULL;
}

void http_request_destroy(HttpRequest *req) {
    if (!req) return;
    /* Clean up body if dynamically allocated in future implementation */
    req->body = NULL;
    req->body_bytes_read = 0;
}
