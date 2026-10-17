/**
 * @file test_controller.c
 * @brief Unit test suite for Queue-and-Latency-Aware Admission Control (QLAAC).
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "adaptive_controller.h"

static void test_qlaac_worker_saturation_empty_queue(void) {
    QlaacController c;
    QlaacConfig cfg = {
        .enabled = true,
        .queue_caution_threshold = 0.70,
        .queue_overload_threshold = 0.90,
        .latency_caution_ms = 50.0,
        .latency_overload_ms = 150.0,
        .worker_caution_threshold = 0.85,
        .worker_overload_threshold = 1.00,
        .recovery_samples_required = 3,
        .sustained_pressure_threshold = 3
    };

    adaptive_controller_init(&c, &cfg);
    assert(c.current_state == QLAAC_STATE_NORMAL);
    assert(adaptive_controller_evaluate(&c) == QLAAC_ACTION_ADMIT);

    /*
     * Scenario 1: Worker utilization at 100% with only 6% queue occupancy and fast latency (1.5 ms).
     * Rule: Must NOT cause indiscriminate rejection or enter OVERLOAD.
     * Worker saturation serves as an early-warning signal, transitioning to CAUTION.
     */
    QlaacState st = adaptive_controller_update_ex(&c, 0.06, 1.00, 1.5);
    assert(st == QLAAC_STATE_CAUTION);
    /* In CAUTION, incoming requests are admitted; keepalive is throttled */
    assert(adaptive_controller_evaluate(&c) == QLAAC_ACTION_THROTTLE_KEEPALIVE);
    assert(c.total_overload_transitions == 0);
    assert(c.total_shed_requests == 0);

    /* Continued operation under 100% worker saturation with 6% queue occupancy stays in CAUTION */
    st = adaptive_controller_update_ex(&c, 0.06, 1.00, 2.0);
    assert(st == QLAAC_STATE_CAUTION);
    st = adaptive_controller_update_ex(&c, 0.06, 1.00, 2.0);
    assert(st == QLAAC_STATE_CAUTION);
    assert(c.total_overload_transitions == 0);
    assert(c.total_shed_requests == 0);

    adaptive_controller_destroy(&c);
    printf("[PASS] test_qlaac_worker_saturation_empty_queue\n");
}

static void test_qlaac_worker_saturation_growing_queue(void) {
    QlaacController c;
    QlaacConfig cfg = {
        .enabled = true,
        .queue_caution_threshold = 0.70,
        .queue_overload_threshold = 0.90,
        .latency_caution_ms = 50.0,
        .latency_overload_ms = 150.0,
        .worker_caution_threshold = 0.85,
        .worker_overload_threshold = 1.00,
        .recovery_samples_required = 3,
        .sustained_pressure_threshold = 3
    };

    adaptive_controller_init(&c, &cfg);

    /*
     * Scenario 2: Worker utilization at 100% coupled with a growing queue (75% >= 70% caution).
     * Rule: Saturated workers + growing queue exceeds draining capacity -> triggers OVERLOAD.
     */
    QlaacState st = adaptive_controller_update_ex(&c, 0.75, 1.00, 12.0);
    assert(st == QLAAC_STATE_OVERLOAD);
    assert(adaptive_controller_evaluate(&c) == QLAAC_ACTION_REJECT_OVERLOAD);
    assert(c.total_overload_transitions == 1);
    assert(c.total_shed_requests == 1);

    adaptive_controller_destroy(&c);
    printf("[PASS] test_qlaac_worker_saturation_growing_queue\n");
}

static void test_qlaac_sustained_high_latency(void) {
    QlaacController c;
    QlaacConfig cfg = {
        .enabled = true,
        .queue_caution_threshold = 0.70,
        .queue_overload_threshold = 0.90,
        .latency_caution_ms = 50.0,
        .latency_overload_ms = 150.0,
        .worker_caution_threshold = 0.85,
        .worker_overload_threshold = 1.00,
        .recovery_samples_required = 3,
        .sustained_pressure_threshold = 3
    };

    adaptive_controller_init(&c, &cfg);

    /*
     * Scenario 3: Low queue (10%) and moderate worker utilization (50%),
     * but p95 latency breaches SLA threshold (180 ms >= 150 ms).
     * Rule: Severe tail latency triggers OVERLOAD.
     */
    QlaacState st = adaptive_controller_update_ex(&c, 0.10, 0.50, 180.0);
    assert(st == QLAAC_STATE_OVERLOAD);
    assert(adaptive_controller_evaluate(&c) == QLAAC_ACTION_REJECT_OVERLOAD);
    assert(c.total_overload_transitions == 1);

    adaptive_controller_destroy(&c);
    printf("[PASS] test_qlaac_sustained_high_latency\n");
}

static void test_qlaac_sustained_pressure(void) {
    QlaacController c;
    QlaacConfig cfg = {
        .enabled = true,
        .queue_caution_threshold = 0.70,
        .queue_overload_threshold = 0.90,
        .latency_caution_ms = 50.0,
        .latency_overload_ms = 150.0,
        .worker_caution_threshold = 0.85,
        .worker_overload_threshold = 1.00,
        .recovery_samples_required = 3,
        .sustained_pressure_threshold = 3
    };

    adaptive_controller_init(&c, &cfg);

    /*
     * Scenario 4: Worker saturation with non-trivial queue backing up (55% >= 50%)
     * across consecutive sampling windows.
     * Window 1 & 2 remain in CAUTION.
     * Window 3 triggers OVERLOAD due to sustained pressure.
     */
    QlaacState st1 = adaptive_controller_update_ex(&c, 0.55, 1.00, 15.0);
    assert(st1 == QLAAC_STATE_CAUTION);
    assert(adaptive_controller_evaluate(&c) == QLAAC_ACTION_THROTTLE_KEEPALIVE);

    QlaacState st2 = adaptive_controller_update_ex(&c, 0.55, 1.00, 18.0);
    assert(st2 == QLAAC_STATE_CAUTION);

    QlaacState st3 = adaptive_controller_update_ex(&c, 0.55, 1.00, 20.0);
    assert(st3 == QLAAC_STATE_OVERLOAD);
    assert(adaptive_controller_evaluate(&c) == QLAAC_ACTION_REJECT_OVERLOAD);
    assert(c.total_overload_transitions == 1);

    adaptive_controller_destroy(&c);
    printf("[PASS] test_qlaac_sustained_pressure\n");
}

static void test_qlaac_recovery(void) {
    QlaacController c;
    QlaacConfig cfg = {
        .enabled = true,
        .queue_caution_threshold = 0.70,
        .queue_overload_threshold = 0.90,
        .latency_caution_ms = 50.0,
        .latency_overload_ms = 150.0,
        .worker_caution_threshold = 0.85,
        .worker_overload_threshold = 1.00,
        .recovery_samples_required = 3,
        .sustained_pressure_threshold = 3
    };

    adaptive_controller_init(&c, &cfg);

    /* Push into OVERLOAD via critical queue occupancy */
    QlaacState st = adaptive_controller_update_ex(&c, 0.95, 0.90, 25.0);
    assert(st == QLAAC_STATE_OVERLOAD);

    /*
     * Hysteresis Recovery:
     * 3 consecutive healthy samples required to step down OVERLOAD -> CAUTION.
     */
    st = adaptive_controller_update_ex(&c, 0.10, 0.20, 5.0);
    assert(st == QLAAC_STATE_OVERLOAD);
    st = adaptive_controller_update_ex(&c, 0.10, 0.20, 5.0);
    assert(st == QLAAC_STATE_OVERLOAD);
    st = adaptive_controller_update_ex(&c, 0.10, 0.20, 5.0);
    assert(st == QLAAC_STATE_CAUTION);

    /*
     * 3 more consecutive healthy samples required to step down CAUTION -> NORMAL.
     */
    st = adaptive_controller_update_ex(&c, 0.10, 0.20, 5.0);
    assert(st == QLAAC_STATE_CAUTION);
    st = adaptive_controller_update_ex(&c, 0.10, 0.20, 5.0);
    assert(st == QLAAC_STATE_CAUTION);
    st = adaptive_controller_update_ex(&c, 0.10, 0.20, 5.0);
    assert(st == QLAAC_STATE_NORMAL);
    assert(adaptive_controller_evaluate(&c) == QLAAC_ACTION_ADMIT);

    adaptive_controller_destroy(&c);
    printf("[PASS] test_qlaac_recovery\n");
}

static void test_qlaac_hysteresis_reset(void) {
    QlaacController c;
    QlaacConfig cfg = {
        .enabled = true,
        .queue_caution_threshold = 0.70,
        .queue_overload_threshold = 0.90,
        .latency_caution_ms = 50.0,
        .latency_overload_ms = 150.0,
        .worker_caution_threshold = 0.85,
        .worker_overload_threshold = 1.00,
        .recovery_samples_required = 3,
        .sustained_pressure_threshold = 3
    };

    adaptive_controller_init(&c, &cfg);

    /* Push into OVERLOAD */
    QlaacState st = adaptive_controller_update_ex(&c, 0.95, 0.50, 20.0);
    assert(st == QLAAC_STATE_OVERLOAD);

    /* 2 healthy samples received */
    st = adaptive_controller_update_ex(&c, 0.20, 0.20, 10.0);
    assert(st == QLAAC_STATE_OVERLOAD);
    st = adaptive_controller_update_ex(&c, 0.20, 0.20, 10.0);
    assert(st == QLAAC_STATE_OVERLOAD);

    /* Sudden spike resets healthy counter to 0 */
    st = adaptive_controller_update_ex(&c, 0.95, 0.50, 20.0);
    assert(st == QLAAC_STATE_OVERLOAD);

    /* Now requires 3 full healthy samples consecutively to step down */
    st = adaptive_controller_update_ex(&c, 0.20, 0.20, 10.0);
    assert(st == QLAAC_STATE_OVERLOAD);
    st = adaptive_controller_update_ex(&c, 0.20, 0.20, 10.0);
    assert(st == QLAAC_STATE_OVERLOAD);
    st = adaptive_controller_update_ex(&c, 0.20, 0.20, 10.0);
    assert(st == QLAAC_STATE_CAUTION);

    adaptive_controller_destroy(&c);
    printf("[PASS] test_qlaac_hysteresis_reset\n");
}

static void test_qlaac_disabled_policy(void) {
    QlaacController c;
    QlaacConfig cfg = {
        .enabled = false,
        .queue_caution_threshold = 0.70,
        .queue_overload_threshold = 0.90,
        .latency_caution_ms = 50.0,
        .latency_overload_ms = 150.0,
        .worker_caution_threshold = 0.85,
        .worker_overload_threshold = 1.00,
        .recovery_samples_required = 3,
        .sustained_pressure_threshold = 3
    };

    adaptive_controller_init(&c, &cfg);
    assert(c.current_state == QLAAC_STATE_NORMAL);

    /* Even with maximum queue, 100% workers, and 500ms latency, disabled controller stays NORMAL */
    QlaacState st = adaptive_controller_update_ex(&c, 1.0, 1.0, 500.0);
    assert(st == QLAAC_STATE_NORMAL);
    assert(adaptive_controller_evaluate(&c) == QLAAC_ACTION_ADMIT);

    adaptive_controller_destroy(&c);
    printf("[PASS] test_qlaac_disabled_policy\n");
}

int main(void) {
    printf("--- Running QLAAC Controller Unit Tests ---\n");
    test_qlaac_worker_saturation_empty_queue();
    test_qlaac_worker_saturation_growing_queue();
    test_qlaac_sustained_high_latency();
    test_qlaac_sustained_pressure();
    test_qlaac_recovery();
    test_qlaac_hysteresis_reset();
    test_qlaac_disabled_policy();
    printf("--- All QLAAC Controller Tests Passed ---\n");
    return EXIT_SUCCESS;
}
