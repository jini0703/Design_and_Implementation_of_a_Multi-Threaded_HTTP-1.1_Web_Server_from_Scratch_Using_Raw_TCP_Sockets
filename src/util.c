/**
 * @file util.c
 * @brief Monotonic clocks, timestamped logging, and JSON escaping.
 */

#include "util.h"

#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static atomic_int g_log_level = LOG_LEVEL_INFO;

uint64_t monotonic_ns(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * NS_PER_SEC + (uint64_t)ts.tv_nsec;
}

uint64_t monotonic_ms(void)
{
    return monotonic_ns() / NS_PER_MS;
}

void wall_clock_iso8601(char *buf, size_t cap)
{
    if (buf == NULL || cap == 0) {
        return;
    }
    struct timespec ts;
    struct tm tm_utc;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0 || gmtime_r(&ts.tv_sec, &tm_utc) == NULL) {
        snprintf(buf, cap, "unknown-time");
        return;
    }
    char base[32];
    if (strftime(base, sizeof base, "%Y-%m-%dT%H:%M:%S", &tm_utc) == 0) {
        snprintf(buf, cap, "unknown-time");
        return;
    }
    snprintf(buf, cap, "%s.%03ldZ", base, ts.tv_nsec / 1000000L);
}

void log_set_level(LogLevel level)
{
    atomic_store(&g_log_level, (int)level);
}

void log_msg(LogLevel level, const char *fmt, ...)
{
    if ((int)level < atomic_load(&g_log_level)) {
        return;
    }
    static const char *const names[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    const char *name = ((int)level >= 0 && (int)level <= 3) ? names[level] : "LOG";

    char stamp[48];
    wall_clock_iso8601(stamp, sizeof stamp);

    /* Build the whole line first so concurrent threads do not interleave fragments. */
    char line[1024];
    int prefix = snprintf(line, sizeof line, "%s [%s] ", stamp, name);
    if (prefix < 0 || (size_t)prefix >= sizeof line) {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    int body = vsnprintf(line + prefix, sizeof line - (size_t)prefix, fmt, ap);
    va_end(ap);
    if (body < 0) {
        return;
    }
    size_t used = strnlen(line, sizeof line - 2);
    line[used] = '\n';
    line[used + 1] = '\0';
    fputs(line, stderr);
}

int json_escape(const char *in, char *out, size_t cap)
{
    if (out == NULL || cap == 0) {
        return -1;
    }
    size_t o = 0;
    if (in == NULL) {
        out[0] = '\0';
        return 0;
    }
    for (const unsigned char *p = (const unsigned char *)in; *p != '\0'; ++p) {
        char tmp[8];
        const char *piece = tmp;
        size_t plen;
        switch (*p) {
        case '"':  piece = "\\\""; plen = 2; break;
        case '\\': piece = "\\\\"; plen = 2; break;
        case '\n': piece = "\\n";  plen = 2; break;
        case '\r': piece = "\\r";  plen = 2; break;
        case '\t': piece = "\\t";  plen = 2; break;
        default:
            if (*p < 0x20 || *p == 0x7F) {
                snprintf(tmp, sizeof tmp, "\\u%04x", (unsigned)*p);
                plen = 6;
            } else {
                tmp[0] = (char)*p;
                tmp[1] = '\0';
                plen = 1;
            }
            break;
        }
        if (o + plen >= cap) {
            out[o] = '\0';
            return -1;
        }
        memcpy(out + o, piece, plen);
        o += plen;
    }
    out[o] = '\0';
    return 0;
}
