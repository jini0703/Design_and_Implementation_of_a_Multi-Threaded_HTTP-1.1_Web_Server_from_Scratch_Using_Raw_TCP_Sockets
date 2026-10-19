/**
 * @file adaptive_controller.c
 * @brief Queue-and-Latency-Aware Admission Control (QLAAC) experimental overload controller.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include "adaptive_controller.h"
#include "util.h"
#include <string.h>

void adaptive_controller_init(QlaacController *controller, const QlaacConfig *config) {
    if (!controller) return;
    memset(controller, 0, sizeof(QlaacController));
    if (config) {
        controller->config = *config;
    }
    if (controller->config.sustained_pressure_threshold == 0) {
        controller->config.sustained_pressure_threshold = 3;
    }
    if (controller->config.recovery_samples_required == 0) {
        controller->config.recovery_samples_required = 3;
    }
    if (controller->config.queue_wait_caution_ms <= 0.0) {
        controller->config.queue_wait_caution_ms = 25.0;
    }
    if (controller->config.queue_wait_overload_ms <= 0.0) {
        controller->config.queue_wait_overload_ms = 100.0;
    }
    controller->current_state = QLAAC_STATE_NORMAL;
    controller->consecutive_healthy_samples = 0;
    controller->consecutive_pressure_samples = 0;
    controller->total_overload_transitions = 0;
    controller->total_shed_requests = 0;
    pthread_mutex_init(&controller->lock, NULL);
}

const char *adaptive_controller_state_name(QlaacState state) {
    switch (state) {
        case QLAAC_STATE_NORMAL:   return "NORMAL";
        case QLAAC_STATE_CAUTION:  return "CAUTION";
        case QLAAC_STATE_OVERLOAD: return "OVERLOAD";
        default:                   return "UNKNOWN";
    }
}

QlaacState adaptive_controller_update_queue_aware(QlaacController *controller,
                                                  double queue_occupancy_ratio,
                                                  double worker_utilization_ratio,
                                                  double p95_processing_latency_ms,
                                                  double p95_queue_wait_ms) {
    if (!controller) return QLAAC_STATE_NORMAL;
    if (!controller->config.enabled) return QLAAC_STATE_NORMAL;

    pthread_mutex_lock(&controller->lock);

    const QlaacConfig *cfg = &controller->config;
    QlaacState old_state = controller->current_state;

    bool worker_saturated = (cfg->worker_overload_threshold > 0.0 &&
                             worker_utilization_ratio >= cfg->worker_overload_threshold);
    bool worker_caution = (cfg->worker_caution_threshold > 0.0 &&
                           worker_utilization_ratio >= cfg->worker_caution_threshold);
    bool queue_growing = (cfg->queue_caution_threshold > 0.0 &&
                          queue_occupancy_ratio >= cfg->queue_caution_threshold);
    bool queue_critical = (cfg->queue_overload_threshold > 0.0 &&
                           queue_occupancy_ratio >= cfg->queue_overload_threshold);
    bool latency_elevated = (cfg->latency_caution_ms > 0.0 &&
                             p95_processing_latency_ms > 0.0 &&
                             p95_processing_latency_ms >= cfg->latency_caution_ms);
    bool latency_critical = (cfg->latency_overload_ms > 0.0 &&
                             p95_processing_latency_ms > 0.0 &&
                             p95_processing_latency_ms >= cfg->latency_overload_ms);
    /*
     * Queue-wait duration represents active operational pressure only when
     * connections are actually waiting in the queue (occupancy >= 20%).
     * When the queue has drained, lingering reservoir samples from past bursts
     * must not block recovery or falsely trigger overload.
     */
    bool has_queue_presence = (queue_occupancy_ratio >= 0.20);
    bool queue_wait_elevated = (cfg->queue_wait_caution_ms > 0.0 &&
                                p95_queue_wait_ms > 0.0 &&
                                p95_queue_wait_ms >= cfg->queue_wait_caution_ms &&
                                has_queue_presence);
    bool queue_wait_critical = (cfg->queue_wait_overload_ms > 0.0 &&
                                p95_queue_wait_ms > 0.0 &&
                                p95_queue_wait_ms >= cfg->queue_wait_overload_ms &&
                                has_queue_presence);

    /*
     * Track sustained pressure:
     * Compound pressure occurs when:
     * - Workers are saturated with non-trivial queue backing up (>= 50%).
     * - Queue is growing (>= 70%) with elevated queue-wait or processing latency.
     * Note: Saturated workers alone with a moderate queue (< 50%) and healthy latency
     * represents healthy system throughput, not sustained overload pressure.
     */
    bool sustained_pressure_candidate =
        (worker_saturated && queue_occupancy_ratio >= 0.50) ||
        (queue_growing && (queue_wait_elevated || latency_elevated));

    if (sustained_pressure_candidate) {
        controller->consecutive_pressure_samples++;
    } else {
        controller->consecutive_pressure_samples = 0;
    }

    size_t req_pressure = cfg->sustained_pressure_threshold > 0 ? cfg->sustained_pressure_threshold : 3;
    bool sustained_pressure_overload = (controller->consecutive_pressure_samples >= req_pressure);

    /*
     * OVERLOAD Condition:
     * 1. Critical queue occupancy (queue buffer almost full >= 90%).
     * 2. Critical processing latency (SLA breach >= 150 ms).
     * 3. Worker saturation coupled with a growing queue (queue >= 70%).
     * 4. Sustained compound pressure over multiple consecutive sampling windows.
     * 5. Critical queue-wait latency when queue is backing up (>= 70%) or
     *    when already in OVERLOAD (blocking premature recovery).
     *
     * Note: When starting from NORMAL or CAUTION, critical queue-wait with a moderate
     * queue (< 70%) enters CAUTION first, applying keep-alive fair sharing to rotate
     * workers and drain the queue without shedding traffic.
     */
    bool queue_wait_overload = queue_wait_critical &&
                               (queue_growing || (controller->current_state == QLAAC_STATE_OVERLOAD));

    bool is_overload_condition = queue_critical ||
                                 latency_critical ||
                                 queue_wait_overload ||
                                 (worker_saturated && queue_growing) ||
                                 sustained_pressure_overload;

    /*
     * CAUTION Condition:
     * Evidence of actual queue/latency pressure:
     * 1. Queue occupancy >= caution threshold (>= 70%).
     * 2. Observed queue-wait duration >= caution threshold (>= 25 ms).
     * 3. Processing latency >= caution threshold (>= 50 ms).
     * 4. Worker saturation/caution coupled with non-trivial queue depth (>= 50%).
     *
     * Rule: High worker utilization alone when the queue is short (< 50%) and
     * queue wait and latency are healthy MUST REMAIN in NORMAL.
     */
    bool worker_stress_with_queue = (worker_caution || worker_saturated) &&
                                    (queue_occupancy_ratio >= 0.50 || queue_wait_elevated || latency_elevated);

    bool is_caution_condition = is_overload_condition ||
                                queue_growing ||
                                queue_wait_elevated ||
                                queue_wait_critical ||
                                latency_elevated ||
                                worker_stress_with_queue;

    if (is_overload_condition) {
        if (controller->current_state != QLAAC_STATE_OVERLOAD) {
            controller->total_overload_transitions++;
            controller->current_state = QLAAC_STATE_OVERLOAD;
            log_msg(LOG_LEVEL_WARN,
                    "[QLAAC] Transition %s -> OVERLOAD (queue=%.2f, worker_util=%.2f, proc_p95=%.2f ms, qw_p95=%.2f ms, sustained=%zu)",
                    adaptive_controller_state_name(old_state),
                    queue_occupancy_ratio, worker_utilization_ratio,
                    p95_processing_latency_ms, p95_queue_wait_ms,
                    controller->consecutive_pressure_samples);
        }
        controller->consecutive_healthy_samples = 0;
    } else if (is_caution_condition) {
        if (controller->current_state == QLAAC_STATE_OVERLOAD) {
            /* Require sustained recovery windows before stepping down from OVERLOAD to CAUTION */
            controller->consecutive_healthy_samples++;
            if (controller->consecutive_healthy_samples >= cfg->recovery_samples_required) {
                controller->current_state = QLAAC_STATE_CAUTION;
                controller->consecutive_healthy_samples = 0;
                log_msg(LOG_LEVEL_INFO,
                        "[QLAAC] Recovery transition OVERLOAD -> CAUTION (queue=%.2f, worker_util=%.2f, proc_p95=%.2f ms, qw_p95=%.2f ms)",
                        queue_occupancy_ratio, worker_utilization_ratio,
                        p95_processing_latency_ms, p95_queue_wait_ms);
            }
        } else if (controller->current_state == QLAAC_STATE_NORMAL) {
            controller->current_state = QLAAC_STATE_CAUTION;
            controller->consecutive_healthy_samples = 0;
            log_msg(LOG_LEVEL_WARN,
                    "[QLAAC] Transition NORMAL -> CAUTION (queue=%.2f, worker_util=%.2f, proc_p95=%.2f ms, qw_p95=%.2f ms)",
                    queue_occupancy_ratio, worker_utilization_ratio,
                    p95_processing_latency_ms, p95_queue_wait_ms);
        } else {
            /* Already in CAUTION */
            controller->consecutive_healthy_samples = 0;
        }
    } else {
        /* Pressure is completely below caution thresholds */
        if (controller->current_state == QLAAC_STATE_OVERLOAD) {
            controller->consecutive_healthy_samples++;
            if (controller->consecutive_healthy_samples >= cfg->recovery_samples_required) {
                controller->current_state = QLAAC_STATE_CAUTION;
                controller->consecutive_healthy_samples = 0;
                log_msg(LOG_LEVEL_INFO,
                        "[QLAAC] Recovery transition OVERLOAD -> CAUTION (queue=%.2f, worker_util=%.2f, proc_p95=%.2f ms, qw_p95=%.2f ms)",
                        queue_occupancy_ratio, worker_utilization_ratio,
                        p95_processing_latency_ms, p95_queue_wait_ms);
            }
        } else if (controller->current_state == QLAAC_STATE_CAUTION) {
            controller->consecutive_healthy_samples++;
            if (controller->consecutive_healthy_samples >= cfg->recovery_samples_required) {
                controller->current_state = QLAAC_STATE_NORMAL;
                controller->consecutive_healthy_samples = 0;
                log_msg(LOG_LEVEL_INFO,
                        "[QLAAC] Recovery transition CAUTION -> NORMAL (queue=%.2f, worker_util=%.2f, proc_p95=%.2f ms, qw_p95=%.2f ms)",
                        queue_occupancy_ratio, worker_utilization_ratio,
                        p95_processing_latency_ms, p95_queue_wait_ms);
            }
        } else {
            controller->current_state = QLAAC_STATE_NORMAL;
            controller->consecutive_healthy_samples = 0;
        }
    }

    QlaacState state = controller->current_state;
    pthread_mutex_unlock(&controller->lock);
    return state;
}

QlaacState adaptive_controller_update_ex(QlaacController *controller,
                                         double queue_occupancy_ratio,
                                         double worker_utilization_ratio,
                                         double p95_latency_ms) {
    return adaptive_controller_update_queue_aware(controller,
                                                  queue_occupancy_ratio,
                                                  worker_utilization_ratio,
                                                  p95_latency_ms,
                                                  0.0);
}

QlaacState adaptive_controller_update(QlaacController *controller,
                                      double queue_occupancy_ratio,
                                      double p95_latency_ms) {
    return adaptive_controller_update_queue_aware(controller,
                                                  queue_occupancy_ratio,
                                                  0.0,
                                                  p95_latency_ms,
                                                  0.0);
}

QlaacDecision adaptive_controller_evaluate(QlaacController *controller) {
    if (!controller || !controller->config.enabled) {
        return QLAAC_ACTION_ADMIT;
    }

    pthread_mutex_lock(&controller->lock);
    QlaacState state = controller->current_state;
    QlaacDecision decision = QLAAC_ACTION_ADMIT;

    if (state == QLAAC_STATE_OVERLOAD) {
        controller->total_shed_requests++;
        decision = QLAAC_ACTION_REJECT_OVERLOAD;
    } else if (state == QLAAC_STATE_CAUTION) {
        decision = QLAAC_ACTION_THROTTLE_KEEPALIVE;
    } else {
        decision = QLAAC_ACTION_ADMIT;
    }
    pthread_mutex_unlock(&controller->lock);

    return decision;
}

QlaacState adaptive_controller_get_state(QlaacController *controller) {
    if (!controller || !controller->config.enabled) {
        return QLAAC_STATE_NORMAL;
    }

    pthread_mutex_lock(&controller->lock);
    QlaacState state = controller->current_state;
    pthread_mutex_unlock(&controller->lock);

    return state;
}

void adaptive_controller_get_snapshot(QlaacController *controller,
                                      QlaacState *state_out,
                                      uint64_t *overload_transitions_out,
                                      uint64_t *shed_requests_out) {
    if (!controller) {
        if (state_out) *state_out = QLAAC_STATE_NORMAL;
        if (overload_transitions_out) *overload_transitions_out = 0;
        if (shed_requests_out) *shed_requests_out = 0;
        return;
    }

    pthread_mutex_lock(&controller->lock);
    if (state_out) *state_out = controller->current_state;
    if (overload_transitions_out) *overload_transitions_out = controller->total_overload_transitions;
    if (shed_requests_out) *shed_requests_out = controller->total_shed_requests;
    pthread_mutex_unlock(&controller->lock);
}

void adaptive_controller_destroy(QlaacController *controller) {
    if (!controller) return;
    pthread_mutex_destroy(&controller->lock);
}
