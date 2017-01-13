#include "../logging_native.h"

void debug(const char *fmt, ...) {
    if (utox_verbosity() < VERBOSITY_DEBUG) {
        return;
    }
    va_list l;
    va_start(l, fmt);
    NSLogv(@(fmt), l);
    va_end(l);
}

void debug_info(const char *fmt, ...) {
    if (utox_verbosity() < VERBOSITY_INFO) {
        return;
    }
    va_list l;
    va_start(l, fmt);
    NSLogv(@(fmt), l);
    va_end(l);
}

void debug_notice(const char *fmt, ...) {
    if (utox_verbosity() < VERBOSITY_NOTICE) {
        return;
    }
    va_list l;
    va_start(l, fmt);
    NSLogv(@(fmt), l);
    va_end(l);
}

void debug_error(const char *fmt, ...) {
    if (utox_verbosity() < VERBOSITY_ERROR) {
        return;
    }
    va_list l;
    va_start(l, fmt);
    NSLogv(@(fmt), l);
    va_end(l);
}