/**
 * @file test_queue.c
 * @brief Unit test suite for bounded connection queue and concurrency controls.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "connection_queue.h"

static void test_queue_push_pop(void) {
    ConnectionQueue q;
    int rc = connection_queue_init(&q, 4);
    assert(rc == 0);
    assert(connection_queue_get_count(&q) == 0);
    assert(connection_queue_occupancy_ratio(&q) == 0.0);

    ClientConnection c1 = { .fd = 101, .port = 8081 };
    strncpy(c1.ip, "127.0.0.1", sizeof(c1.ip) - 1);
    ClientConnection c2 = { .fd = 102, .port = 8082 };
    strncpy(c2.ip, "127.0.0.1", sizeof(c2.ip) - 1);

    rc = connection_queue_push(&q, &c1, 0);
    assert(rc == 0);
    assert(connection_queue_get_count(&q) == 1);
    assert(connection_queue_occupancy_ratio(&q) == 0.25);

    rc = connection_queue_push(&q, &c2, 0);
    assert(rc == 0);
    assert(connection_queue_get_count(&q) == 2);
    assert(connection_queue_occupancy_ratio(&q) == 0.50);

    ClientConnection out1;
    rc = connection_queue_pop(&q, &out1, 0);
    assert(rc == 0);
    assert(out1.fd == 101);
    assert(connection_queue_get_count(&q) == 1);

    ClientConnection out2;
    rc = connection_queue_pop(&q, &out2, 0);
    assert(rc == 0);
    assert(out2.fd == 102);
    assert(connection_queue_get_count(&q) == 0);

    connection_queue_destroy(&q);
    printf("[PASS] test_queue_push_pop\n");
}

static void test_queue_capacity_limit(void) {
    ConnectionQueue q;
    connection_queue_init(&q, 2);

    ClientConnection c = { .fd = -1, .port = 80 };
    assert(connection_queue_push(&q, &c, 0) == 0);
    assert(connection_queue_push(&q, &c, 0) == 0);

    /* Queue is now full (2/2); non-blocking push should fail with -1 */
    assert(connection_queue_push(&q, &c, 0) == -1);

    connection_queue_destroy(&q);
    printf("[PASS] test_queue_capacity_limit\n");
}

static void test_queue_shutdown(void) {
    ConnectionQueue q;
    connection_queue_init(&q, 4);

    connection_queue_shutdown(&q);

    ClientConnection c = { .fd = -1 };
    /* Push should return -2 when shut down */
    assert(connection_queue_push(&q, &c, 0) == -2);

    ClientConnection out;
    /* Pop should return -2 when empty and shut down */
    assert(connection_queue_pop(&q, &out, 0) == -2);

    connection_queue_destroy(&q);
    printf("[PASS] test_queue_shutdown\n");
}

int main(void) {
    printf("--- Running Connection Queue Unit Tests ---\n");
    test_queue_push_pop();
    test_queue_capacity_limit();
    test_queue_shutdown();
    printf("--- All Connection Queue Tests Passed ---\n");
    return EXIT_SUCCESS;
}
