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
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "http_response.h"
#include "tcp_socket.h"
#include "util.h"

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

static void test_response_build_error_cleanup(void) {
    HttpResponse res;
    http_response_init(&res);

    /* Allocate dummy body */
    http_response_set_body_copy(&res, "text/plain", "temporary payload", 17);
    assert(res.owns_body == true && res.body != NULL);

    /* Attach a valid dummy file descriptor */
    int dummy_fd = dup(fileno(stdout));
    assert(dummy_fd >= 0);
    res.file_fd = dummy_fd;

    /* Calling build_error must destroy previous body and close file_fd */
    http_response_build_error(&res, HTTP_STATUS_INTERNAL_ERROR, "test error", false);
    assert(res.status == HTTP_STATUS_INTERNAL_ERROR);
    assert(res.file_fd == -1);
    assert(res.body != NULL);
    assert(res.owns_body == true);
    assert(strstr(res.body, "test error") != NULL);

    /* Verify dummy_fd was closed */
    assert(close(dummy_fd) == -1); /* Should fail with EBADF because it was already closed */

    http_response_destroy(&res);
    printf("[PASS] test_response_build_error_cleanup\n");
}

static void test_sendfile_fallback_offset_integrity(void) {
    char tmp_path[] = "/tmp/test_sendfile_XXXXXX";
    int tfd = mkstemp(tmp_path);
    assert(tfd >= 0);

    const char *payload = "0123456789ABCDEF";
    assert(write(tfd, payload, 16) == 16);
    close(tfd);

    int file_fd = open(tmp_path, O_RDONLY);
    assert(file_fd >= 0);

    int sv[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

    size_t sent_out = 0;
    uint64_t deadline = monotonic_ns() + 1000000000ULL;
    int rc = tcp_sendfile_all(sv[0], file_fd, 16, deadline, false, &sent_out);
    assert(rc == 0);
    assert(sent_out == 16);

    char recv_buf[32];
    memset(recv_buf, 0, sizeof(recv_buf));
    ssize_t r = read(sv[1], recv_buf, sizeof(recv_buf));
    assert(r == 16);
    assert(memcmp(recv_buf, payload, 16) == 0);

    /* Test fallback with offset: seek to offset 8 and transmit remaining 8 bytes */
    assert(lseek(file_fd, 8, SEEK_SET) == 8);
    rc = tcp_sendfile_all(sv[0], file_fd, 8, deadline, false, &sent_out);
    assert(rc == 0);
    assert(sent_out == 8);

    memset(recv_buf, 0, sizeof(recv_buf));
    r = read(sv[1], recv_buf, sizeof(recv_buf));
    assert(r == 8);
    assert(memcmp(recv_buf, "89ABCDEF", 8) == 0);

    close(sv[0]);
    close(sv[1]);
    close(file_fd);
    unlink(tmp_path);

    printf("[PASS] test_sendfile_fallback_offset_integrity\n");
}

int main(void) {
    printf("--- Running HTTP Response Unit Tests ---\n");
    test_status_phrases();
    test_response_init();
    test_response_build_error_cleanup();
    test_sendfile_fallback_offset_integrity();
    printf("--- All HTTP Response Tests Passed ---\n");
    return EXIT_SUCCESS;
}
