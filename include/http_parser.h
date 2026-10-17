#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

/**
 * @file http_parser.h
 * @brief Zero-third-party RFC 9112 compliant HTTP/1.1 streaming request parser.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Implements strict RFC 9112 request parsing:
 *  - Request line: Method SP Request-URI SP HTTP-Version CRLF
 *  - Case-insensitive header name matching
 *  - Required HTTP/1.1 Host header validation (RFC 9112 §7.1.2)
 *  - Framing: Content-Length validation, integer bounds, duplicate/conflicting header rejection
 *  - Transfer-Encoding detection and deliberate rejection (chunked not implemented -> 501)
 *  - Expect: 100-continue detection (417 Expectation Failed)
 *  - Persistent connection semantics (Keep-Alive by default in 1.1, Connection: close)
 *  - Preserves leftover unread bytes for sequential pipelined requests
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HTTP_MAX_HEADERS       64
#define HTTP_MAX_HEADER_NAME   128
#define HTTP_MAX_HEADER_VALUE  2048
#define HTTP_MAX_URI_LENGTH    2048
#define DEFAULT_MAX_HEADER_SZ  16384
#define DEFAULT_MAX_BODY_SZ    10485760  /* 10 MB */

/**
 * @brief Supported HTTP Methods according to project scope.
 */
typedef enum {
    HTTP_METHOD_UNKNOWN = 0,
    HTTP_METHOD_GET,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_TRACE,
    HTTP_METHOD_CONNECT,
    HTTP_METHOD_PATCH
} HttpMethod;

/**
 * @brief Parsing result classifications for defensive rejection and routing.
 */
typedef enum {
    PARSER_OK = 0,
    PARSER_NEED_MORE_DATA,
    PARSER_ERR_BAD_REQUEST,            /* 400 Bad Request (syntax error, missing Host, duplicate CL) */
    PARSER_ERR_METHOD_NOT_ALLOWED,      /* 405 Method Not Allowed */
    PARSER_ERR_NOT_IMPLEMENTED,         /* 501 Not Implemented (unsupported method, transfer-encoding) */
    PARSER_ERR_URI_TOO_LONG,            /* 414 URI Too Long */
    PARSER_ERR_HEADERS_TOO_LARGE,       /* 431 Request Header Fields Too Large */
    PARSER_ERR_BODY_TOO_LARGE,          /* 413 Payload Too Large */
    PARSER_ERR_VERSION_NOT_SUPPORTED,   /* 505 HTTP Version Not Supported */
    PARSER_ERR_EXPECTATION_FAILED,      /* 417 Expectation Failed */
    PARSER_ERR_LENGTH_REQUIRED          /* 411 Length Required */
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
    char path[HTTP_MAX_URI_LENGTH];     /* Path component only (e.g. "/index.html") */
    char query[HTTP_MAX_URI_LENGTH];    /* Query string without '?' */
    char http_version[16];
    bool is_http_11;
    bool is_http_10;
    HttpHeader headers[HTTP_MAX_HEADERS];
    size_t header_count;
    char host[256];
    bool keep_alive;
    bool has_content_length;
    int64_t content_length;
    bool has_transfer_encoding;
    bool has_expect_100;
    char *body;                         /* Points into input buffer or dynamically allocated */
    size_t body_len;
    bool owns_body;
    size_t total_request_bytes;         /* Total bytes consumed from stream for this request */
} HttpRequest;

/**
 * @brief Per-connection streaming input buffer for persistent TCP connections.
 */
typedef struct {
    char *buf;
    size_t capacity;
    size_t length;
} ConnectionBuffer;

/**
 * @brief Initialize a connection buffer with designated capacity.
 */
int conn_buffer_init(ConnectionBuffer *cb, size_t capacity);

/**
 * @brief Discard n bytes from the front of the connection buffer and shift unconsumed bytes.
 */
void conn_buffer_consume(ConnectionBuffer *cb, size_t n);

/**
 * @brief Free resources in a connection buffer.
 */
void conn_buffer_free(ConnectionBuffer *cb);

/**
 * @brief Reset an HttpRequest structure to default empty state.
 */
void http_request_init(HttpRequest *req);

/**
 * @brief Look up an HTTP header value case-insensitively.
 * @param req Parsed request.
 * @param name Header name to search (e.g. "Content-Type", "Host").
 * @return Pointer to header value string, or NULL if header not present.
 */
const char *http_request_get_header(const HttpRequest *req, const char *name);

/**
 * @brief Parse a single HTTP request from a raw byte buffer.
 *
 * @param raw_data Incoming byte stream.
 * @param length Total bytes available in raw_data.
 * @param max_header_size Maximum allowed header block size in bytes.
 * @param max_body_size Maximum allowed request body size in bytes.
 * @param req Output parsed request struct.
 * @param bytes_consumed Out parameter receiving total bytes consumed (headers + body).
 * @return ParserStatus code.
 */
ParserStatus http_parse_request(const char *raw_data, size_t length,
                               size_t max_header_size, size_t max_body_size,
                               HttpRequest *req, size_t *bytes_consumed);

/**
 * @brief Convert ParserStatus error code into corresponding HTTP status code.
 */
int parser_status_to_http_code(ParserStatus status);

/**
 * @brief Free any dynamically allocated resources within the HttpRequest.
 */
void http_request_destroy(HttpRequest *req);

#endif /* HTTP_PARSER_H */
