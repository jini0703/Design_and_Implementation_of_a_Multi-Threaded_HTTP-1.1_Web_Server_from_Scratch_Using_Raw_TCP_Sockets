#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

/**
 * @file http_response.h
 * @brief HTTP/1.1 response formatting, status codes, standard headers, and network transmission.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include "http_parser.h"

/**
 * @brief Standard HTTP Status Codes supported across the server.
 */
typedef enum {
    HTTP_STATUS_OK                    = 200,
    HTTP_STATUS_CREATED               = 201,
    HTTP_STATUS_NO_CONTENT            = 204,
    HTTP_STATUS_MOVED_PERMANENTLY     = 301,
    HTTP_STATUS_NOT_MODIFIED          = 304,
    HTTP_STATUS_BAD_REQUEST           = 400,
    HTTP_STATUS_FORBIDDEN             = 403,
    HTTP_STATUS_NOT_FOUND             = 404,
    HTTP_STATUS_METHOD_NOT_ALLOWED    = 405,
    HTTP_STATUS_REQUEST_TIMEOUT       = 408,
    HTTP_STATUS_LENGTH_REQUIRED       = 411,
    HTTP_STATUS_PAYLOAD_TOO_LARGE     = 413,
    HTTP_STATUS_URI_TOO_LONG          = 414,
    HTTP_STATUS_EXPECTATION_FAILED    = 417,
    HTTP_STATUS_HEADERS_TOO_LARGE     = 431,
    HTTP_STATUS_INTERNAL_ERROR        = 500,
    HTTP_STATUS_NOT_IMPLEMENTED       = 501,
    HTTP_STATUS_SERVICE_UNAVAILABLE   = 503,
    HTTP_STATUS_VERSION_NOT_SUPPORTED = 505
} HttpStatus;

/**
 * @brief Complete HTTP Response representation.
 */
typedef struct {
    HttpStatus status;
    HttpHeader headers[HTTP_MAX_HEADERS];
    size_t header_count;
    char *body;
    size_t body_len;
    bool owns_body;
    bool is_head_only;           /* If true, headers reflect representation but body is omitted */
    int file_fd;                 /* Open file descriptor for sendfile streaming (-1 if memory body) */
    size_t file_size;
    bool is_keep_alive;
} HttpResponse;

/**
 * @brief Retrieve standard RFC reason phrase for a given HTTP status code.
 */
const char *http_status_reason_phrase(HttpStatus status);

/**
 * @brief Initialize an HttpResponse struct to clean default state.
 */
void http_response_init(HttpResponse *res);

/**
 * @brief Set status code on response.
 */
void http_response_set_status(HttpResponse *res, HttpStatus status);

/**
 * @brief Add or replace a response header.
 */
int http_response_add_header(HttpResponse *res, const char *name, const char *value);

/**
 * @brief Assign an in-memory body to response (copied into allocated memory).
 */
int http_response_set_body_copy(HttpResponse *res, const char *content_type, const void *data, size_t len);

/**
 * @brief Assign an in-memory body to response (takes ownership without copying).
 */
int http_response_set_body_nocopy(HttpResponse *res, const char *content_type, void *data, size_t len, bool take_ownership);

/**
 * @brief Configure zero-copy or streaming file delivery.
 */
void http_response_set_file(HttpResponse *res, const char *content_type, int file_fd, size_t file_size);

/**
 * @brief Construct a uniform, standard HTML error response.
 */
void http_response_build_error(HttpResponse *res, HttpStatus status, const char *custom_message, bool keep_alive);

/**
 * @brief Serialize response status-line and headers into a contiguous buffer.
 */
ssize_t http_response_serialize_headers(const HttpResponse *res, char *buffer, size_t buffer_size);

/**
 * @brief Transmit entire HTTP response (headers + body or file) over socket descriptor.
 *
 * @param client_fd Target TCP socket.
 * @param res Response structure.
 * @param deadline_ns Absolute deadline for completing transmission.
 * @param use_sendfile Whether to enable zero-copy sendfile(2) for file payloads.
 * @param bytes_sent Out parameter receiving total bytes transferred.
 * @return 0 on success, negative TCP_ERR_* code on socket error or timeout.
 */
int http_response_send(int client_fd, HttpResponse *res, uint64_t deadline_ns,
                       bool use_sendfile, size_t *bytes_sent);

/**
 * @brief Release memory and file descriptors held by HttpResponse.
 */
void http_response_destroy(HttpResponse *res);

#endif /* HTTP_RESPONSE_H */
