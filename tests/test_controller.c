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
        .sustained_pressure_threshold = 3,
        .queue_wait_caution_ms = 25.0,
        .queue_wait_overload_ms = 100.0
    };

    adaptive_controller_init(&c, &cfg);
    assert(c.current_state == QLAAC_STATE_NORMAL);
    assert(adaptive_controller_evaluate(&c) == QLAAC_ACTION_ADMIT);

    /*
     * Scenario 1: Worker utilization at 100% with only 6% queue occupancy, fast latency (1.5 ms),
     * and healthy queue wait (0.5 ms).
     * Rule: High worker utilization alone with a short queue and low queue wait MUST NOT
     * trigger restrictive CAUTION state. Processing continues normally in NORMAL.
     */
    QlaacState st = adaptive_controller_update_queue_aware(&c, 0.06, 1.00, 1.5, 0.5);
    assert(st == QLAAC_STATE_NORMAL);
    assert(adaptive_controller_evaluate(&c) == QLAAC_ACTION_ADMIT);
    assert(c.total_overload_transitions == 0);
    assert(c.total_shed_requests == 0);

    /* Continued operation under 100% worker saturation with short queue stays in NORMAL */
    st = adaptive_controller_update_queue_aware(&c, 0.06, 1.00, 2.0, 0.5);
    assert(st == QLAAC_STATE_NORMAL);
    st = adaptive_controller_update_queue_aware(&c, 0.06, 1.00, 2.0, 0.6);
    assert(st == QLAAC_STATE_NORMAL);
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

static void test_qlaac_get_state_no_side_effects(void) {
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
    assert(adaptive_controller_get_state(&c) == QLAAC_STATE_NORMAL);

    /* Trigger OVERLOAD state */
    QlaacState st = adaptive_controller_update_ex(&c, 0.95, 1.00, 200.0);
    assert(st == QLAAC_STATE_OVERLOAD);

    /* Querying state must return OVERLOAD without incrementing total_shed_requests */
    for (int i = 0; i < 10; i++) {
        assert(adaptive_controller_get_state(&c) == QLAAC_STATE_OVERLOAD);
    }
    assert(c.total_shed_requests == 0);

    /* Admission evaluate call DOES increment total_shed_requests */
    assert(adaptive_controller_evaluate(&c) == QLAAC_ACTION_REJECT_OVERLOAD);
    assert(c.total_shed_requests == 1);

    /* Subsequent get_state queries still do not increment shed counter */
    for (int i = 0; i < 5; i++) {
        assert(adaptive_controller_get_state(&c) == QLAAC_STATE_OVERLOAD);
    }
    assert(c.total_shed_requests == 1);

    adaptive_controller_destroy(&c);
    printf("[PASS] test_qlaac_get_state_no_side_effects\n");
}

static void test_qlaac_get_snapshot_thread_safety(void) {
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

    /* Initial snapshot check */
    QlaacState s;
    uint64_t trans, shed;
    adaptive_controller_get_snapshot(&c, &s, &trans, &shed);
    assert(s == QLAAC_STATE_NORMAL);
    assert(trans == 0);
    assert(shed == 0);

    /* Safe handling of NULL pointers */
    adaptive_controller_get_snapshot(&c, NULL, NULL, NULL);

    /* Trigger OVERLOAD */
    adaptive_controller_update_ex(&c, 0.95, 1.00, 200.0);
    adaptive_controller_evaluate(&c); /* Increments shed counter */

    for (int i = 0; i < 10; i++) {
        adaptive_controller_get_snapshot(&c, &s, &trans, &shed);
        assert(s == QLAAC_STATE_OVERLOAD);
        assert(trans == 1);
        assert(shed == 1);
    }

    adaptive_controller_destroy(&c);
    printf("[PASS] test_qlaac_get_snapshot_thread_safety\n");
}

static void test_qlaac_worker_saturation_moderate_queue(void) {
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
        .sustained_pressure_threshold = 3,
        .queue_wait_caution_ms = 25.0,
        .queue_wait_overload_ms = 100.0
    };

    adaptive_controller_init(&c, &cfg);

    /* 100% workers + queue occupancy 55% (>= 50%) -> enters CAUTION */
    QlaacState st = adaptive_controller_update_queue_aware(&c, 0.55, 1.00, 15.0, 5.0);
    assert(st == QLAAC_STATE_CAUTION);
    assert(adaptive_controller_evaluate(&c) == QLAAC_ACTION_THROTTLE_KEEPALIVE);

    adaptive_controller_destroy(&c);
    printf("[PASS] test_qlaac_worker_saturation_moderate_queue\n");
}

static void test_qlaac_queue_wait_triggers(void) {
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
        .sustained_pressure_threshold = 3,
        .queue_wait_caution_ms = 25.0,
        .queue_wait_overload_ms = 100.0
    };

    adaptive_controller_init(&c, &cfg);

    /* Queue wait >= 25.0 ms triggers CAUTION even with short queue (20%) and low worker load (40%) */
    QlaacState st = adaptive_controller_update_queue_aware(&c, 0.20, 0.40, 5.0, 30.0);
    assert(st == QLAAC_STATE_CAUTION);
    assert(adaptive_controller_evaluate(&c) == QLAAC_ACTION_THROTTLE_KEEPALIVE);

    /* Queue wait >= 100.0 ms with moderate queue (< 70%) triggers CAUTION (fair-sharing, no shedding) */
    st = adaptive_controller_update_queue_aware(&c, 0.40, 0.40, 5.0, 120.0);
    assert(st == QLAAC_STATE_CAUTION);
    assert(adaptive_controller_evaluate(&c) == QLAAC_ACTION_THROTTLE_KEEPALIVE);

    /* Queue wait >= 100.0 ms coupled with growing queue (>= 70%) triggers OVERLOAD (shedding) */
    st = adaptive_controller_update_queue_aware(&c, 0.75, 0.40, 5.0, 120.0);
    assert(st == QLAAC_STATE_OVERLOAD);
    assert(adaptive_controller_evaluate(&c) == QLAAC_ACTION_REJECT_OVERLOAD);

    adaptive_controller_destroy(&c);
    printf("[PASS] test_qlaac_queue_wait_triggers\n");
}

static void test_qlaac_c32_moderate_queue_no_premature_overload(void) {
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
        .sustained_pressure_threshold = 3,
        .queue_wait_caution_ms = 25.0,
        .queue_wait_overload_ms = 100.0
    };

    adaptive_controller_init(&c, &cfg);

    /*
     * Regression test for C=32 moderate queue scenario:
     * 28/64 queued connections (occupancy 0.4375 < 0.70), 100% worker utilization,
     * ultra-fast processing latency (0.15 ms), initial queue-wait = 240 ms.
     * The controller MUST enter CAUTION (to activate keep-alive fair sharing)
     * and MUST NOT trigger OVERLOAD (which would falsely shed 90% of requests).
     */
    QlaacState st = adaptive_controller_update_queue_aware(&c, 0.4375, 1.00, 0.15, 240.0);
    assert(st == QLAAC_STATE_CAUTION);
    assert(adaptive_controller_evaluate(&c) == QLAAC_ACTION_THROTTLE_KEEPALIVE);
    assert(c.total_shed_requests == 0);
    assert(c.total_overload_transitions == 0);

    adaptive_controller_destroy(&c);
    printf("[PASS] test_qlaac_c32_moderate_queue_no_premature_overload\n");
}

static void test_qlaac_recovery_blocked_by_queue_wait(void) {
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
        .sustained_pressure_threshold = 3,
        .queue_wait_caution_ms = 25.0,
        .queue_wait_overload_ms = 100.0
    };

    adaptive_controller_init(&c, &cfg);

    /* Push into OVERLOAD via high latency */
    QlaacState st = adaptive_controller_update_queue_aware(&c, 0.20, 0.50, 180.0, 10.0);
    assert(st == QLAAC_STATE_OVERLOAD);

    /* While queue-wait remains critical (110 ms >= 100 ms), it CANNOT step down from OVERLOAD */
    for (int i = 0; i < 5; i++) {
        st = adaptive_controller_update_queue_aware(&c, 0.20, 0.50, 2.0, 110.0);
        assert(st == QLAAC_STATE_OVERLOAD);
    }

    /* Queue-wait drops to 40 ms (caution level < 100 ms but >= 25 ms).
     * 3 consecutive samples allow step down from OVERLOAD to CAUTION. */
    st = adaptive_controller_update_queue_aware(&c, 0.20, 0.50, 2.0, 40.0);
    assert(st == QLAAC_STATE_OVERLOAD);
    st = adaptive_controller_update_queue_aware(&c, 0.20, 0.50, 2.0, 40.0);
    assert(st == QLAAC_STATE_OVERLOAD);
    st = adaptive_controller_update_queue_aware(&c, 0.20, 0.50, 2.0, 40.0);
    assert(st == QLAAC_STATE_CAUTION);

    /* But while queue-wait is 40 ms (>= 25 ms caution threshold), it CANNOT recover to NORMAL */
    for (int i = 0; i < 5; i++) {
        st = adaptive_controller_update_queue_aware(&c, 0.20, 0.50, 2.0, 40.0);
        assert(st == QLAAC_STATE_CAUTION);
    }

    /* Now queue wait drops to 1.0 ms (truly healthy) for 3 consecutive samples -> recovers to NORMAL */
    st = adaptive_controller_update_queue_aware(&c, 0.20, 0.50, 2.0, 1.0);
    assert(st == QLAAC_STATE_CAUTION);
    st = adaptive_controller_update_queue_aware(&c, 0.20, 0.50, 2.0, 1.0);
    assert(st == QLAAC_STATE_CAUTION);
    st = adaptive_controller_update_queue_aware(&c, 0.20, 0.50, 2.0, 1.0);
    assert(st == QLAAC_STATE_NORMAL);
    assert(adaptive_controller_evaluate(&c) == QLAAC_ACTION_ADMIT);

    adaptive_controller_destroy(&c);
    printf("[PASS] test_qlaac_recovery_blocked_by_queue_wait\n");
}

int main(void) {
    printf("--- Running QLAAC Controller Unit Tests ---\n");
    test_qlaac_worker_saturation_empty_queue();
    test_qlaac_worker_saturation_moderate_queue();
    test_qlaac_worker_saturation_growing_queue();
    test_qlaac_sustained_high_latency();
    test_qlaac_sustained_pressure();
    test_qlaac_queue_wait_triggers();
    test_qlaac_c32_moderate_queue_no_premature_overload();
    test_qlaac_recovery();
    test_qlaac_recovery_blocked_by_queue_wait();
    test_qlaac_hysteresis_reset();
    test_qlaac_disabled_policy();
    test_qlaac_get_state_no_side_effects();
    test_qlaac_get_snapshot_thread_safety();
    printf("--- All QLAAC Controller Tests Passed ---\n");
    return EXIT_SUCCESS;
}
