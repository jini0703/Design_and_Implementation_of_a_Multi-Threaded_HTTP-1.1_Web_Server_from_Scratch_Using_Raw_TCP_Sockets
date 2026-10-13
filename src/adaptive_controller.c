/**
 * @file adaptive_controller.c
 * @brief Queue-and-Latency-Aware Admission Control (QLAAC) experimental overload controller.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Intended Responsibilities:
 *  1. Adaptive Overload Feedback Loop:
 *     - Monitor real-time queue occupancy ratio and rolling 95th-percentile response latency.
 *  2. Three-Tier State Machine:
 *     - NORMAL: Standard operation, full admission, persistent connections permitted.
 *     - CAUTION: Load elevated. Shorten keep-alive timeouts and signal clients to close idle connections.
 *     - OVERLOAD: Capacity saturated or latency SLA breached. Immediately shed excess incoming connections
 *       with HTTP 503 (Service Unavailable) before workers become overwhelmed.
 *  3. Hysteresis & Oscillation Dampening:
 *     - Require a configurable streak of consecutive healthy sampling periods (recovery_samples_required)
 *       before stepping down from OVERLOAD to CAUTION, or CAUTION to NORMAL.
 *  4. Experimental Telemetry:
 *     - Log state transitions, timestamped overload events, and shedding statistics for empirical analysis.
 *
 * Implementation Status:
 *  - Architecture specified in include/adaptive_controller.h.
 *  - State machine update logic scheduled for development in Phase 4.
 */

#include "adaptive_controller.h"
#include <string.h>

void adaptive_controller_init(QlaacController *controller, const QlaacConfig *config) {
    if (!controller) return;
    memset(controller, 0, sizeof(QlaacController));
    if (config) {
        controller->config = *config;
    }
    controller->current_state = QLAAC_STATE_NORMAL;
    controller->consecutive_healthy_samples = 0;
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

QlaacDecision adaptive_controller_evaluate(QlaacController *controller) {
    if (!controller || !controller->config.enabled) {
        return QLAAC_ACTION_ADMIT;
    }

    pthread_mutex_lock(&controller->lock);
    QlaacState state = controller->current_state;
    pthread_mutex_unlock(&controller->lock);

    if (state == QLAAC_STATE_OVERLOAD) {
        return QLAAC_ACTION_REJECT_OVERLOAD;
    } else if (state == QLAAC_STATE_CAUTION) {
        return QLAAC_ACTION_THROTTLE_KEEPALIVE;
    }
    return QLAAC_ACTION_ADMIT;
}

void adaptive_controller_destroy(QlaacController *controller) {
    if (!controller) return;
    pthread_mutex_destroy(&controller->lock);
}
