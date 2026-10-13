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

int main(void) {
    printf("--- Running HTTP Parser Unit Tests ---\n");
    test_request_init();
    test_header_case_insensitive_lookup();
    printf("--- All HTTP Parser Tests Passed ---\n");
    return EXIT_SUCCESS;
}
