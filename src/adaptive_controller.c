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

QlaacState adaptive_controller_update_ex(QlaacController *controller,
                                         double queue_occupancy_ratio,
                                         double worker_utilization_ratio,
                                         double p95_latency_ms) {
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
                             p95_latency_ms > 0.0 &&
                             p95_latency_ms >= cfg->latency_caution_ms);
    bool latency_critical = (cfg->latency_overload_ms > 0.0 &&
                             p95_latency_ms > 0.0 &&
                             p95_latency_ms >= cfg->latency_overload_ms);

    /*
     * Track sustained pressure:
     * Moderate stress occurs when workers are saturated with non-trivial queue depth (>= 50%),
     * when workers are saturated with elevated latency, or when both queue and latency are elevated.
     * Note: Worker saturation alone with nearly empty queue (< 50%) and low latency (< caution)
     * is an early warning signal, not sustained overload pressure.
     */
    bool sustained_pressure_candidate = (worker_saturated && queue_occupancy_ratio >= 0.50) ||
                                        (worker_saturated && latency_elevated) ||
                                        (queue_growing && latency_elevated);

    if (sustained_pressure_candidate) {
        controller->consecutive_pressure_samples++;
    } else {
        controller->consecutive_pressure_samples = 0;
    }

    size_t req_pressure = cfg->sustained_pressure_threshold > 0 ? cfg->sustained_pressure_threshold : 3;
    bool sustained_pressure_overload = (controller->consecutive_pressure_samples >= req_pressure);

    /*
     * OVERLOAD Condition:
     * 1. Critical queue occupancy (queue buffer almost full).
     * 2. Critical p95 latency (SLA breach).
     * 3. Worker saturation coupled with a growing queue (queue >= caution threshold).
     * 4. Worker saturation coupled with elevated tail latency (latency >= caution threshold).
     * 5. Sustained pressure over multiple consecutive sampling windows.
     *
     * Note: Worker saturation alone with a nearly empty queue and low p95 latency
     * acts as an early warning signal (CAUTION) and does NOT trigger OVERLOAD.
     */
    bool is_overload_condition = queue_critical ||
                                 latency_critical ||
                                 (worker_saturated && queue_growing) ||
                                 (worker_saturated && latency_elevated) ||
                                 sustained_pressure_overload;

    /*
     * CAUTION Condition:
     * Worker utilization at or above caution threshold, worker saturation alone,
     * queue occupancy >= caution threshold, or p95 latency >= caution threshold.
     */
    bool is_caution_condition = is_overload_condition ||
                                queue_growing ||
                                latency_elevated ||
                                worker_caution ||
                                worker_saturated;

    if (is_overload_condition) {
        if (controller->current_state != QLAAC_STATE_OVERLOAD) {
            controller->total_overload_transitions++;
            controller->current_state = QLAAC_STATE_OVERLOAD;
            log_msg(LOG_LEVEL_WARN,
                    "[QLAAC] Transition %s -> OVERLOAD (queue=%.2f, worker_util=%.2f, p95=%.2f ms, sustained=%zu)",
                    adaptive_controller_state_name(old_state),
                    queue_occupancy_ratio, worker_utilization_ratio, p95_latency_ms,
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
                        "[QLAAC] Recovery transition OVERLOAD -> CAUTION (queue=%.2f, worker_util=%.2f, p95=%.2f ms)",
                        queue_occupancy_ratio, worker_utilization_ratio, p95_latency_ms);
            }
        } else if (controller->current_state == QLAAC_STATE_NORMAL) {
            controller->current_state = QLAAC_STATE_CAUTION;
            controller->consecutive_healthy_samples = 0;
            log_msg(LOG_LEVEL_WARN,
                    "[QLAAC] Transition NORMAL -> CAUTION (queue=%.2f, worker_util=%.2f, p95=%.2f ms)",
                    queue_occupancy_ratio, worker_utilization_ratio, p95_latency_ms);
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
                        "[QLAAC] Recovery transition OVERLOAD -> CAUTION (queue=%.2f, worker_util=%.2f, p95=%.2f ms)",
                        queue_occupancy_ratio, worker_utilization_ratio, p95_latency_ms);
            }
        } else if (controller->current_state == QLAAC_STATE_CAUTION) {
            controller->consecutive_healthy_samples++;
            if (controller->consecutive_healthy_samples >= cfg->recovery_samples_required) {
                controller->current_state = QLAAC_STATE_NORMAL;
                controller->consecutive_healthy_samples = 0;
                log_msg(LOG_LEVEL_INFO,
                        "[QLAAC] Recovery transition CAUTION -> NORMAL (queue=%.2f, worker_util=%.2f, p95=%.2f ms)",
                        queue_occupancy_ratio, worker_utilization_ratio, p95_latency_ms);
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

QlaacState adaptive_controller_update(QlaacController *controller,
                                      double queue_occupancy_ratio,
                                      double p95_latency_ms) {
    return adaptive_controller_update_ex(controller, queue_occupancy_ratio, 0.0, p95_latency_ms);
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
