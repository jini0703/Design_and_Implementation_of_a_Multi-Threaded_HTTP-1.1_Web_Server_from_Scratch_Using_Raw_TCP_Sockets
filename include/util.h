#ifndef UTIL_H
#define UTIL_H

/**
 * @file util.h
 * @brief Small shared helpers: monotonic/wall clocks, logging, JSON string escaping.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * All elapsed-time measurements in the server use CLOCK_MONOTONIC so that wall-clock
 * adjustments (NTP, manual changes) cannot distort latency, timeout, or controller logic.
 * The wall clock is used only for human-readable log timestamps and HTTP Date headers.
 */

#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define UTIL_PRINTF_LIKE(fmt_idx, arg_idx) __attribute__((format(printf, fmt_idx, arg_idx)))
#else
#define UTIL_PRINTF_LIKE(fmt_idx, arg_idx)
#endif

#define NS_PER_US  UINT64_C(1000)
#define NS_PER_MS  UINT64_C(1000000)
#define NS_PER_SEC UINT64_C(1000000000)

/** @return Nanoseconds from an arbitrary fixed point (CLOCK_MONOTONIC). */
uint64_t monotonic_ns(void);

/** @return Milliseconds from an arbitrary fixed point (CLOCK_MONOTONIC). */
uint64_t monotonic_ms(void);

/** Write the current UTC time as "YYYY-MM-DDTHH:MM:SS.mmmZ" into buf (always NUL-terminated). */
void wall_clock_iso8601(char *buf, size_t cap);

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel;

/** Set the minimum level that log_msg() will print (default: LOG_LEVEL_INFO). */
void log_set_level(LogLevel level);

/** Thread-safe single-line log message to stderr with an ISO-8601 UTC timestamp. */
void log_msg(LogLevel level, const char *fmt, ...) UTIL_PRINTF_LIKE(2, 3);

/**
 * Escape a string for inclusion inside a JSON string literal (quotes not added).
 * @return 0 if the complete escaped string fit into out, -1 if it was truncated.
 *         out is always NUL-terminated when cap > 0.
 */
int json_escape(const char *in, char *out, size_t cap);

#endif /* UTIL_H */
