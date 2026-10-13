#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

/**
 * @file http_parser.h
 * @brief Zero-third-party RFC 9112 compliant HTTP/1.1 request parser.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Module Responsibilities:
 *  - Incrementally parse HTTP/1.1 request bytes arriving over TCP byte streams.
 *  - Strictly validate Request-Line syntax: Method SP Request-URI SP HTTP-Version CRLF.
 *  - Extract and validate HTTP header fields (Field-Name: OWS Field-Value OWS CRLF).
 *  - Provide case-insensitive header lookup (RFC 9112 section 5.1).
 *  - Detect and parse message framing: Content-Length validation and bounds checking.
 *  - Handle persistent connection indicators (Connection: keep-alive vs. close).
 *  - Defensively guard against oversized request lines, header buffer overflows,
 *    and malformed formatting with descriptive error codes.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define HTTP_MAX_HEADERS       64
#define HTTP_MAX_HEADER_NAME   128
#define HTTP_MAX_HEADER_VALUE  1024
#define HTTP_MAX_URI_LENGTH    2048

/**
 * @brief Supported HTTP Methods according to project scope.
 */
typedef enum {
    HTTP_METHOD_UNKNOWN = 0,
    HTTP_METHOD_GET,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_POST
} HttpMethod;

/**
 * @brief Parser execution and state machine progression.
 */
typedef enum {
    PARSER_STATE_REQUEST_LINE = 0,
    PARSER_STATE_HEADERS,
    PARSER_STATE_BODY,
    PARSER_STATE_COMPLETE,
    PARSER_STATE_ERROR
} ParserState;

/**
 * @brief Parsing error classifications for defensive rejection.
 */
typedef enum {
    PARSER_SUCCESS = 0,
    PARSER_INCOMPLETE,
    PARSER_ERR_METHOD_INVALID,
    PARSER_ERR_URI_TOO_LONG,
    PARSER_ERR_VERSION_UNSUPPORTED,
    PARSER_ERR_MALFORMED_HEADER,
    PARSER_ERR_HEADERS_OVERFLOW,
    PARSER_ERR_INVALID_CONTENT_LENGTH,
    PARSER_ERR_BODY_EXCEEDS_LIMIT
} ParserStatus;

/**
 * @brief Representation of an individual HTTP Header.
 */
typedef struct {
    char name[HTTP_MAX_HEADER_NAME];
    char value[HTTP_MAX_HEADER_VALUE];
} HttpHeader;

/**
 * @brief Parsed HTTP/1.1 Request representation.
 */
typedef struct {
    HttpMethod method;
    char method_raw[16];
    char uri[HTTP_MAX_URI_LENGTH];
    char http_version[16];
    HttpHeader headers[HTTP_MAX_HEADERS];
    size_t header_count;
    bool keep_alive;
    int64_t content_length;
    char *body;
    size_t body_bytes_read;
} HttpRequest;

/**
 * @brief Incremental streaming parser context.
 */
typedef struct {
    ParserState state;
    size_t bytes_consumed;
    size_t max_header_bytes;
    size_t max_body_bytes;
} HttpParserContext;

/**
 * @brief Initialize parser context and limits.
 * @param ctx Parser context to initialize.
 * @param max_header_bytes Maximum allowed byte size for all headers combined.
 * @param max_body_bytes Maximum allowed body size.
 */
void http_parser_init(HttpParserContext *ctx, size_t max_header_bytes, size_t max_body_bytes);

/**
 * @brief Reset an HttpRequest structure to default empty state.
 * @param req Request struct to clear.
 */
void http_request_init(HttpRequest *req);

/**
 * @brief Feed raw TCP stream bytes into the parser.
 * @param ctx Parser context.
 * @param req Request structure being populated.
 * @param raw_bytes Incoming stream buffer.
 * @param length Length of buffer.
 * @return ParserStatus code (PARSER_SUCCESS, PARSER_INCOMPLETE, or error code).
 */
ParserStatus http_parser_consume(HttpParserContext *ctx, HttpRequest *req,
                                 const char *raw_bytes, size_t length);

/**
 * @brief Look up an HTTP header value case-insensitively.
 * @param req Parsed request.
 * @param name Header name to search (e.g. "Content-Type", "Host").
 * @return Pointer to header value string, or NULL if header not present.
 */
const char *http_request_get_header(const HttpRequest *req, const char *name);

/**
 * @brief Free any dynamically allocated resources within the HttpRequest (e.g., body).
 * @param req Request to free.
 */
void http_request_destroy(HttpRequest *req);

#endif /* HTTP_PARSER_H */
