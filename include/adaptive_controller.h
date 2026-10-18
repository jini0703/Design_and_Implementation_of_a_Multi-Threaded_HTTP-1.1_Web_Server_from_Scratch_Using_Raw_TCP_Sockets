#ifndef ADAPTIVE_CONTROLLER_H
#define ADAPTIVE_CONTROLLER_H

/**
 * @file adaptive_controller.h
 * @brief Queue-and-Latency-Aware Admission Control (QLAAC) experimental controller.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Module Responsibilities:
 *  - Implement the experimental Queue-and-Latency-Aware Admission Control (QLAAC) state machine.
 *  - Ingest queue occupancy, worker utilization, and p95 latency.
 *  - Transition between three operational states:
 *      * QLAAC_STATE_NORMAL: Unrestricted standard connection processing within queue bounds.
 *      * QLAAC_STATE_CAUTION: Pre-emptive tightening of keep-alive limits and timeouts.
 *      * QLAAC_STATE_OVERLOAD: Rapid shedding of new connections (immediate HTTP 503 Service Unavailable).
 *  - Enforce hysteresis dampening to prevent oscillation during bursty traffic.
 *  - Log state transitions cleanly without hot-path overhead.
 */

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include "metrics.h"

/**
 * @brief QLAAC Operating States.
 */
typedef enum {
    QLAAC_STATE_NORMAL = 0,
    QLAAC_STATE_CAUTION,
    QLAAC_STATE_OVERLOAD
} QlaacState;

/**
 * @brief Admission Control Decision for an incoming connection or request.
 */
typedef enum {
    QLAAC_ACTION_ADMIT = 0,
    QLAAC_ACTION_THROTTLE_KEEPALIVE,
    QLAAC_ACTION_REJECT_OVERLOAD
} QlaacDecision;

/**
 * @brief QLAAC Controller Configuration.
 */
typedef struct {
    bool enabled;
    double queue_caution_threshold;   /**< e.g. 0.70 (70% full) */
    double queue_overload_threshold;  /**< e.g. 0.90 (90% full) */
    double latency_caution_ms;        /**< e.g. 50.0 ms p95 */
    double latency_overload_ms;       /**< e.g. 150.0 ms p95 */
    double worker_caution_threshold;  /**< e.g. 0.85 (85% busy) */
    double worker_overload_threshold; /**< e.g. 1.00 (100% busy) */
    size_t recovery_samples_required; /**< e.g. 5 consecutive healthy windows */
    size_t sustained_pressure_threshold; /**< e.g. 3 consecutive pressure windows to enter overload */
} QlaacConfig;

/**
 * @brief QLAAC Controller runtime state.
 */
typedef struct {
    QlaacConfig config;
    QlaacState current_state;
    size_t consecutive_healthy_samples;
    size_t consecutive_pressure_samples;
    uint64_t total_overload_transitions;
    uint64_t total_shed_requests;
    pthread_mutex_t lock;
} QlaacController;

/**
 * @brief Initialize the QLAAC adaptive controller.
 * @param controller Controller struct.
 * @param config Configuration parameters.
 */
void adaptive_controller_init(QlaacController *controller, const QlaacConfig *config);

/**
 * @brief Ingest recent metric measurements and evaluate state machine transitions.
 * @param controller Controller struct.
 * @param queue_occupancy_ratio Ratio of current queue depth to capacity (0.0 - 1.0).
 * @param p95_latency_ms Recent 95th-percentile response latency in milliseconds.
 * @return Newly computed or current QlaacState.
 */
QlaacState adaptive_controller_update(QlaacController *controller,
                                      double queue_occupancy_ratio,
                                      double p95_latency_ms);

/**
 * @brief Ingest full metrics including worker utilization and evaluate transitions.
 * @param controller Controller struct.
 * @param queue_occupancy_ratio Ratio of current queue depth to capacity (0.0 - 1.0).
 * @param worker_utilization_ratio Ratio of active workers to total workers (0.0 - 1.0).
 * @param p95_latency_ms Recent 95th-percentile response latency in milliseconds.
 * @return Newly computed or current QlaacState.
 */
QlaacState adaptive_controller_update_ex(QlaacController *controller,
                                         double queue_occupancy_ratio,
                                         double worker_utilization_ratio,
                                         double p95_latency_ms);

/**
 * @brief Evaluate an admission decision for an incoming connection under current state.
 * @param controller Controller struct.
 * @return QlaacDecision (ADMIT, THROTTLE_KEEPALIVE, or REJECT_OVERLOAD).
 */
QlaacDecision adaptive_controller_evaluate(QlaacController *controller);

/**
 * @brief Thread-safe query of current QLAAC state without side effects (does not increment shed counter).
 * @param controller Controller struct.
 * @return Current QlaacState (NORMAL, CAUTION, or OVERLOAD).
 */
QlaacState adaptive_controller_get_state(QlaacController *controller);

/**
 * @brief Thread-safe snapshot of controller state and counters without mutating state.
 * @param controller Controller struct.
 * @param state_out Optional destination for current state.
 * @param overload_transitions_out Optional destination for total overload transitions.
 * @param shed_requests_out Optional destination for total shed requests.
 */
void adaptive_controller_get_snapshot(QlaacController *controller,
                                      QlaacState *state_out,
                                      uint64_t *overload_transitions_out,
                                      uint64_t *shed_requests_out);

/**
 * @brief Convert a QlaacState enum to human-readable string.
 * @param state QLAAC state enum.
 * @return String constant ("NORMAL", "CAUTION", "OVERLOAD").
 */
const char *adaptive_controller_state_name(QlaacState state);

/**
 * @brief Destroy controller resources and lock.
 * @param controller Controller struct.
 */
void adaptive_controller_destroy(QlaacController *controller);

#endif /* ADAPTIVE_CONTROLLER_H */
