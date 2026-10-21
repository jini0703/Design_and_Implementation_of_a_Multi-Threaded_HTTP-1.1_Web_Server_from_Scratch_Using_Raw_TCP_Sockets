/**
 * @file test_parser.c
 * @brief Unit test suite for RFC 9112 HTTP/1.1 request parser.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "http_parser.h"

static void test_request_init(void) {
    HttpRequest req;
    http_request_init(&req);

    assert(req.method == HTTP_METHOD_UNKNOWN);
    assert(req.keep_alive == true);
    assert(req.content_length == -1);
    assert(req.header_count == 0);
    assert(req.body == NULL);

    printf("[PASS] test_request_init\n");
}

static void test_header_case_insensitive_lookup(void) {
    HttpRequest req;
    http_request_init(&req);

    strncpy(req.headers[0].name, "Content-Type", HTTP_MAX_HEADER_NAME - 1);
    strncpy(req.headers[0].value, "text/html", HTTP_MAX_HEADER_VALUE - 1);
    req.header_count = 1;

    const char *val1 = http_request_get_header(&req, "content-type");
    assert(val1 != NULL && strcmp(val1, "text/html") == 0);

    const char *val2 = http_request_get_header(&req, "CONTENT-TYPE");
    assert(val2 != NULL && strcmp(val2, "text/html") == 0);

    const char *val3 = http_request_get_header(&req, "Non-Existent");
    assert(val3 == NULL);

    http_request_destroy(&req);
    printf("[PASS] test_header_case_insensitive_lookup\n");
}

static void test_valid_get_request(void) {
    const char *raw = "GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: test\r\n\r\n";
    size_t consumed = 0;
    HttpRequest req;
    http_request_init(&req);

    ParserStatus st = http_parse_request(raw, strlen(raw), 8192, 1048576, &req, &consumed);
    assert(st == PARSER_OK);
    assert(req.method == HTTP_METHOD_GET);
    assert(strcmp(req.uri, "/index.html") == 0);
    assert(strcmp(req.host, "localhost:8080") == 0);
    assert(req.keep_alive == true);
    assert(consumed == strlen(raw));

    http_request_destroy(&req);
    printf("[PASS] test_valid_get_request\n");
}

static void test_missing_host_header(void) {
    const char *no_host = "GET /index.html HTTP/1.1\r\nUser-Agent: test\r\n\r\n";
    size_t consumed = 0;
    HttpRequest req;
    http_request_init(&req);

    ParserStatus st = http_parse_request(no_host, strlen(no_host), 8192, 1048576, &req, &consumed);
    assert(st == PARSER_ERR_BAD_REQUEST);
    assert(parser_status_to_http_code(st) == 400);

    http_request_destroy(&req);
    printf("[PASS] test_missing_host_header\n");
}

static void test_post_with_body(void) {
    const char *post_raw = "POST /api/echo HTTP/1.1\r\nHost: localhost\r\nContent-Length: 11\r\nContent-Type: text/plain\r\n\r\nhello world";
    size_t consumed = 0;
    HttpRequest req;
    http_request_init(&req);

    ParserStatus st = http_parse_request(post_raw, strlen(post_raw), 8192, 1048576, &req, &consumed);
    assert(st == PARSER_OK);
    assert(req.method == HTTP_METHOD_POST);
    assert(req.body_len == 11);
    assert(req.body != NULL);
    assert(memcmp(req.body, "hello world", 11) == 0);
    assert(consumed == strlen(post_raw));

    http_request_destroy(&req);
    printf("[PASS] test_post_with_body\n");
}

static void test_conflicting_framing_headers(void) {
    const char *conflict = "POST /api/echo HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\nTransfer-Encoding: chunked\r\n\r\n12345";
    size_t consumed = 0;
    HttpRequest req;
    http_request_init(&req);

    ParserStatus st = http_parse_request(conflict, strlen(conflict), 8192, 1048576, &req, &consumed);
    assert(st == PARSER_ERR_BAD_REQUEST);

    http_request_destroy(&req);
    printf("[PASS] test_conflicting_framing_headers\n");
}

static void test_unsupported_transfer_encoding(void) {
    const char *chunked = "POST /api/echo HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n";
    size_t consumed = 0;
    HttpRequest req;
    http_request_init(&req);

    ParserStatus st = http_parse_request(chunked, strlen(chunked), 8192, 1048576, &req, &consumed);
    assert(st == PARSER_ERR_NOT_IMPLEMENTED);
    assert(parser_status_to_http_code(st) == 501);

    http_request_destroy(&req);
    printf("[PASS] test_unsupported_transfer_encoding\n");
}

static void test_expect_continue_rejection(void) {
    const char *expect = "POST /api/echo HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\nExpect: 100-continue\r\n\r\n";
    size_t consumed = 0;
    HttpRequest req;
    http_request_init(&req);

    ParserStatus st = http_parse_request(expect, strlen(expect), 8192, 1048576, &req, &consumed);
    assert(st == PARSER_ERR_EXPECTATION_FAILED);
    assert(parser_status_to_http_code(st) == 417);

    http_request_destroy(&req);
    printf("[PASS] test_expect_continue_rejection\n");
}

static void test_pipelined_leftover_preservation(void) {
    const char *pipelined = "GET /first HTTP/1.1\r\nHost: localhost\r\n\r\nGET /second HTTP/1.1\r\nHost: localhost\r\n\r\n";
    size_t consumed1 = 0;
    HttpRequest req1;
    http_request_init(&req1);

    ParserStatus st1 = http_parse_request(pipelined, strlen(pipelined), 8192, 1048576, &req1, &consumed1);
    assert(st1 == PARSER_OK);
    assert(strcmp(req1.uri, "/first") == 0);
    assert(consumed1 < strlen(pipelined));
    http_request_destroy(&req1);

    size_t consumed2 = 0;
    HttpRequest req2;
    http_request_init(&req2);
    ParserStatus st2 = http_parse_request(pipelined + consumed1, strlen(pipelined) - consumed1, 8192, 1048576, &req2, &consumed2);
    assert(st2 == PARSER_OK);
    assert(strcmp(req2.uri, "/second") == 0);
    assert(consumed1 + consumed2 == strlen(pipelined));
    http_request_destroy(&req2);

    printf("[PASS] test_pipelined_leftover_preservation\n");
}

static void test_uri_query_parsing(void) {
    /* 1. Origin-form with query parameter (/metrics?test=1) */
    {
        const char *raw = "GET /metrics?test=1 HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
        size_t consumed = 0;
        HttpRequest req;
        http_request_init(&req);
        ParserStatus st = http_parse_request(raw, strlen(raw), 8192, 1048576, &req, &consumed);
        assert(st == PARSER_OK);
        assert(strcmp(req.uri, "/metrics?test=1") == 0);
        assert(strcmp(req.path, "/metrics") == 0);
        assert(strcmp(req.query, "test=1") == 0);
        http_request_destroy(&req);
    }

    /* 2. Origin-form with query parameter (/health?test=1) */
    {
        const char *raw = "GET /health?test=1 HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
        size_t consumed = 0;
        HttpRequest req;
        http_request_init(&req);
        ParserStatus st = http_parse_request(raw, strlen(raw), 8192, 1048576, &req, &consumed);
        assert(st == PARSER_OK);
        assert(strcmp(req.uri, "/health?test=1") == 0);
        assert(strcmp(req.path, "/health") == 0);
        assert(strcmp(req.query, "test=1") == 0);
        http_request_destroy(&req);
    }

    /* 3. Empty query string (/metrics?) */
    {
        const char *raw = "GET /metrics? HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
        size_t consumed = 0;
        HttpRequest req;
        http_request_init(&req);
        ParserStatus st = http_parse_request(raw, strlen(raw), 8192, 1048576, &req, &consumed);
        assert(st == PARSER_OK);
        assert(strcmp(req.uri, "/metrics?") == 0);
        assert(strcmp(req.path, "/metrics") == 0);
        assert(strcmp(req.query, "") == 0);
        http_request_destroy(&req);
    }

    /* 4. Multiple query parameters and '?' characters (/api/echo?a=1&b=2?c=3) */
    {
        const char *raw = "POST /api/echo?a=1&b=2?c=3 HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 4\r\n\r\necho";
        size_t consumed = 0;
        HttpRequest req;
        http_request_init(&req);
        ParserStatus st = http_parse_request(raw, strlen(raw), 8192, 1048576, &req, &consumed);
        assert(st == PARSER_OK);
        assert(strcmp(req.uri, "/api/echo?a=1&b=2?c=3") == 0);
        assert(strcmp(req.path, "/api/echo") == 0);
        assert(strcmp(req.query, "a=1&b=2?c=3") == 0);
        assert(req.body_len == 4 && memcmp(req.body, "echo", 4) == 0);
        http_request_destroy(&req);
    }

    /* 5. Path without query string (/health) */
    {
        const char *raw = "GET /health HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
        size_t consumed = 0;
        HttpRequest req;
        http_request_init(&req);
        ParserStatus st = http_parse_request(raw, strlen(raw), 8192, 1048576, &req, &consumed);
        assert(st == PARSER_OK);
        assert(strcmp(req.uri, "/health") == 0);
        assert(strcmp(req.path, "/health") == 0);
        assert(strcmp(req.query, "") == 0);
        http_request_destroy(&req);
    }

    /* 6. Absolute-form URI with query string (http://localhost:8080/metrics?test=1) */
    {
        const char *raw = "GET http://localhost:8080/metrics?test=1 HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
        size_t consumed = 0;
        HttpRequest req;
        http_request_init(&req);
        ParserStatus st = http_parse_request(raw, strlen(raw), 8192, 1048576, &req, &consumed);
        assert(st == PARSER_OK);
        assert(strcmp(req.uri, "http://localhost:8080/metrics?test=1") == 0);
        assert(strcmp(req.path, "/metrics") == 0);
        assert(strcmp(req.query, "test=1") == 0);
        http_request_destroy(&req);
    }

    /* 7. Absolute-form URI without query string (http://localhost:8080/health) */
    {
        const char *raw = "GET http://localhost:8080/health HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
        size_t consumed = 0;
        HttpRequest req;
        http_request_init(&req);
        ParserStatus st = http_parse_request(raw, strlen(raw), 8192, 1048576, &req, &consumed);
        assert(st == PARSER_OK);
        assert(strcmp(req.uri, "http://localhost:8080/health") == 0);
        assert(strcmp(req.path, "/health") == 0);
        assert(strcmp(req.query, "") == 0);
        http_request_destroy(&req);
    }

    printf("[PASS] test_uri_query_parsing\n");
}

static void test_bare_newline_in_header_rejected(void) {
    /* RFC 9112 §2.2 prohibits bare CR outside content (must reject with 400); rejecting
     * bare LF within field lines is this server's defensive choice to prevent request
     * smuggling (RFC 9112 §5.2 covers obsolete line folding). */
    const char *raw = "GET / HTTP/1.1\r\nHost: localhost\nX-Smuggled: 1\r\n\r\n";
    size_t consumed = 0;
    HttpRequest req;
    http_request_init(&req);

    ParserStatus st = http_parse_request(raw, strlen(raw), 8192, 1048576, &req, &consumed);
    assert(st == PARSER_ERR_BAD_REQUEST);
    assert(parser_status_to_http_code(st) == 400);

    http_request_destroy(&req);
    printf("[PASS] test_bare_newline_in_header_rejected\n");
}

static void test_oversized_header_field_rejected(void) {
    /* RFC 9110 §5.4 (Field Limits): A field exceeding server-defined limits must return 431 Request Header Fields Too Large (RFC 6585) */
    char long_val[3000];
    memset(long_val, 'A', sizeof(long_val) - 1);
    long_val[sizeof(long_val) - 1] = '\0';

    char raw[4096];
    snprintf(raw, sizeof(raw), "GET / HTTP/1.1\r\nHost: localhost\r\nX-Long: %s\r\n\r\n", long_val);

    size_t consumed = 0;
    HttpRequest req;
    http_request_init(&req);

    ParserStatus st = http_parse_request(raw, strlen(raw), 16384, 1048576, &req, &consumed);
    assert(st == PARSER_ERR_HEADERS_TOO_LARGE);
    assert(parser_status_to_http_code(st) == 431);

    http_request_destroy(&req);
    printf("[PASS] test_oversized_header_field_rejected\n");
}

static void test_signed_content_length_rejected(void) {
    /* RFC 9110 §8.6: Content-Length must contain only decimal digits; '+' is prohibited */
    const char *raw = "POST /api/echo HTTP/1.1\r\nHost: localhost\r\nContent-Length: +5\r\n\r\n12345";
    size_t consumed = 0;
    HttpRequest req;
    http_request_init(&req);

    ParserStatus st = http_parse_request(raw, strlen(raw), 8192, 1048576, &req, &consumed);
    assert(st == PARSER_ERR_BAD_REQUEST);
    assert(parser_status_to_http_code(st) == 400);

    http_request_destroy(&req);
    printf("[PASS] test_signed_content_length_rejected\n");
}

static void test_content_length_grammar_and_overflow(void) {
    /* 1. Trailing characters in Content-Length -> 400 Bad Request */
    {
        const char *raw = "POST /api/echo HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5abc\r\n\r\n12345";
        size_t consumed = 0;
        HttpRequest req;
        http_request_init(&req);
        ParserStatus st = http_parse_request(raw, strlen(raw), 8192, 1048576, &req, &consumed);
        assert(st == PARSER_ERR_BAD_REQUEST);
        http_request_destroy(&req);
    }

    /* 2. Negative Content-Length -> 400 Bad Request */
    {
        const char *raw = "POST /api/echo HTTP/1.1\r\nHost: localhost\r\nContent-Length: -10\r\n\r\n";
        size_t consumed = 0;
        HttpRequest req;
        http_request_init(&req);
        ParserStatus st = http_parse_request(raw, strlen(raw), 8192, 1048576, &req, &consumed);
        assert(st == PARSER_ERR_BAD_REQUEST);
        http_request_destroy(&req);
    }

    /* 3. Empty Content-Length -> 400 Bad Request */
    {
        const char *raw = "POST /api/echo HTTP/1.1\r\nHost: localhost\r\nContent-Length:\r\n\r\n";
        size_t consumed = 0;
        HttpRequest req;
        http_request_init(&req);
        ParserStatus st = http_parse_request(raw, strlen(raw), 8192, 1048576, &req, &consumed);
        assert(st == PARSER_ERR_BAD_REQUEST);
        http_request_destroy(&req);
    }

    /* 4. Huge 50-digit Content-Length overflow -> 413 Body Too Large */
    {
        const char *raw = "POST /api/echo HTTP/1.1\r\nHost: localhost\r\nContent-Length: 99999999999999999999999999999999999999999999999999\r\n\r\n";
        size_t consumed = 0;
        HttpRequest req;
        http_request_init(&req);
        ParserStatus st = http_parse_request(raw, strlen(raw), 8192, 1048576, &req, &consumed);
        assert(st == PARSER_ERR_BODY_TOO_LARGE);
        assert(parser_status_to_http_code(st) == 413);
        http_request_destroy(&req);
    }

    /* 5. Content-Length exceeding configured max_body_size (1000) -> 413 Body Too Large */
    {
        const char *raw = "POST /api/echo HTTP/1.1\r\nHost: localhost\r\nContent-Length: 2000\r\n\r\n";
        size_t consumed = 0;
        HttpRequest req;
        http_request_init(&req);
        ParserStatus st = http_parse_request(raw, strlen(raw), 8192, 1000, &req, &consumed);
        assert(st == PARSER_ERR_BODY_TOO_LARGE);
        assert(parser_status_to_http_code(st) == 413);
        http_request_destroy(&req);
    }

    /* 6. Duplicate Content-Length headers -> 400 Bad Request */
    {
        const char *raw = "POST /api/echo HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\nContent-Length: 5\r\n\r\n12345";
        size_t consumed = 0;
        HttpRequest req;
        http_request_init(&req);
        ParserStatus st = http_parse_request(raw, strlen(raw), 8192, 1048576, &req, &consumed);
        assert(st == PARSER_ERR_BAD_REQUEST);
        http_request_destroy(&req);
    }

    printf("[PASS] test_content_length_grammar_and_overflow\n");
}

int main(void) {
    printf("--- Running HTTP Parser Unit Tests ---\n");
    test_request_init();
    test_header_case_insensitive_lookup();
    test_valid_get_request();
    test_missing_host_header();
    test_post_with_body();
    test_conflicting_framing_headers();
    test_unsupported_transfer_encoding();
    test_expect_continue_rejection();
    test_pipelined_leftover_preservation();
    test_uri_query_parsing();
    test_bare_newline_in_header_rejected();
    test_oversized_header_field_rejected();
    test_signed_content_length_rejected();
    test_content_length_grammar_and_overflow();
    printf("--- All HTTP Parser Tests Passed ---\n");
    return EXIT_SUCCESS;
}
