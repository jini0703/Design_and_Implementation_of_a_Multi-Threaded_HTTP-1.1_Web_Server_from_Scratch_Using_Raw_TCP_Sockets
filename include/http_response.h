#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

/**
 * @file http_response.h
 * @brief HTTP/1.1 response formatting, status codes, header generation, and serialization.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Module Responsibilities:
 *  - Provide standard HTTP status codes and RFC-compliant reason phrases.
 *  - Construct standard HTTP response headers (Date, Server, Content-Type, Content-Length, Connection).
 *  - Manage response bodies in memory or prepare headers for zero-copy file transfer via sendfile(2).
 *  - Serialize complete HTTP response messages into contiguous byte buffers for network I/O.
 *  - Provide rapid constructors for standard error pages (400, 403, 404, 405, 413, 500, 503).
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include "http_parser.h"

/**
 * @brief Standard HTTP Status Codes supported by server response builder.
 */
typedef enum {
    HTTP_STATUS_OK                    = 200,
    HTTP_STATUS_NO_CONTENT            = 204,
    HTTP_STATUS_MOVED_PERMANENTLY     = 301,
    HTTP_STATUS_NOT_MODIFIED          = 304,
    HTTP_STATUS_BAD_REQUEST           = 400,
    HTTP_STATUS_FORBIDDEN             = 403,
    HTTP_STATUS_NOT_FOUND             = 404,
    HTTP_STATUS_METHOD_NOT_ALLOWED    = 405,
    HTTP_STATUS_REQUEST_TIMEOUT       = 408,
    HTTP_STATUS_PAYLOAD_TOO_LARGE     = 413,
    HTTP_STATUS_URI_TOO_LONG          = 414,
    HTTP_STATUS_INTERNAL_ERROR        = 500,
    HTTP_STATUS_SERVICE_UNAVAILABLE   = 503
} HttpStatus;

/**
 * @brief HTTP Response representation.
 */
typedef struct {
    HttpStatus status;
    HttpHeader headers[HTTP_MAX_HEADERS];
    size_t header_count;
    char *body;
    size_t body_len;
    bool is_keep_alive;
    bool is_file_stream;
    int file_fd;
    size_t file_size;
} HttpResponse;

/**
 * @brief Retrieve standard RFC reason phrase for a given HTTP status code.
 * @param status HTTP status enum.
 * @return String constant reason phrase (e.g., "OK", "Not Found", "Service Unavailable").
 */
const char *http_status_reason_phrase(HttpStatus status);

/**
 * @brief Initialize an HttpResponse struct.
 * @param res Response struct to initialize.
 */
void http_response_init(HttpResponse *res);

/**
 * @brief Add a header to an HTTP response.
 * @param res Response struct.
 * @param name Header name.
 * @param value Header value.
 * @return 0 on success, -1 on header overflow.
 */
int http_response_add_header(HttpResponse *res, const char *name, const char *value);

/**
 * @brief Assign an in-memory body to the response.
 * @param res Response struct.
 * @param content_type MIME type string (e.g. "text/html", "application/json").
 * @param body Data buffer (copied or referenced).
 * @param length Byte length of body.
 * @return 0 on success, -1 on failure.
 */
int http_response_set_body(HttpResponse *res, const char *content_type, const char *body, size_t length);

/**
 * @brief Populate a standard defensive error response with friendly HTML error message.
 * @param res Response struct.
 * @param status HTTP error status code.
 * @param custom_message Optional specific diagnostic message, or NULL for standard explanation.
 */
void http_response_build_error(HttpResponse *res, HttpStatus status, const char *custom_message);

/**
 * @brief Serialize response status-line and headers into a contiguous string buffer.
 * @param res Response struct.
 * @param buffer Output buffer.
 * @param buffer_size Capacity of output buffer.
 * @return Number of bytes written to buffer, or -1 if buffer capacity is insufficient.
 */
ssize_t http_response_serialize_headers(const HttpResponse *res, char *buffer, size_t buffer_size);

/**
 * @brief Free resources associated with HttpResponse.
 * @param res Response struct to free.
 */
void http_response_destroy(HttpResponse *res);

#endif /* HTTP_RESPONSE_H */
