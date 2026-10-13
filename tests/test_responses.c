/**
 * @file test_responses.c
 * @brief Unit test suite for HTTP/1.1 response builder and status mappings.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "http_response.h"

static void test_status_phrases(void) {
    assert(strcmp(http_status_reason_phrase(HTTP_STATUS_OK), "OK") == 0);
    assert(strcmp(http_status_reason_phrase(HTTP_STATUS_BAD_REQUEST), "Bad Request") == 0);
    assert(strcmp(http_status_reason_phrase(HTTP_STATUS_NOT_FOUND), "Not Found") == 0);
    assert(strcmp(http_status_reason_phrase(HTTP_STATUS_PAYLOAD_TOO_LARGE), "Payload Too Large") == 0);
    assert(strcmp(http_status_reason_phrase(HTTP_STATUS_SERVICE_UNAVAILABLE), "Service Unavailable") == 0);

    printf("[PASS] test_status_phrases\n");
}

static void test_response_init(void) {
    HttpResponse res;
    http_response_init(&res);

    assert(res.status == HTTP_STATUS_OK);
    assert(res.file_fd == -1);
    assert(res.is_keep_alive == true);
    assert(res.body == NULL);
    assert(res.body_len == 0);

    http_response_destroy(&res);
    printf("[PASS] test_response_init\n");
}

int main(void) {
    printf("--- Running HTTP Response Unit Tests ---\n");
    test_status_phrases();
    test_response_init();
    printf("--- All HTTP Response Tests Passed ---\n");
    return EXIT_SUCCESS;
}
