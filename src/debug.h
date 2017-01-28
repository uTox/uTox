#ifndef DEBUG_H
#define DEBUG_H

/* uTox debug levels */
typedef enum {
    LOG_LVL_OFF,
    LOG_LVL_FATAL,
    LOG_LVL_ERROR,
    LOG_LVL_WARNING,
    LOG_LVL_NOTICE,
    LOG_LVL_INFO,
    LOG_LVL_DEBUG,
    LOG_LVL_NET_TRACE,
} LOG_LVL;

// returns current logging verbosity
int utox_verbosity();

// define debugging macros in a platform specific way

#if defined __WIN32__
#include "windows/logging.h"
#elif defined __ANDROID__
#include "android/logging.h"
#elif defined __OBJC__
#include "cocoa/logging.h"
#else
#include "xlib/logging.h"
#endif

#define VERB(x) (utox_verbosity() >= LOG_LVL_##x)

#define LOG_FATAL(ex, file, str, ...) (VERB(FATAL) ? debug("%s:\t%s\n" ## str, file, __VA_ARGS__ ) & exit(ex): ((void)(0)))

#define LOG_ERROR(file, str, ...)     (VERB(ERROR)     ? debug("%s:\t" str "\n", file, ## __VA_ARGS__ ) : ((void)(0)))
#define LOG_WARN(file, str, ...)      (VERB(WARNING)   ? debug("%s:\t" str "\n", file, ## __VA_ARGS__ ) : ((void)(0)))
#define LOG_NOTE(file, str, ...)      (VERB(NOTICE)    ? debug("%s:\t" str "\n", file, ## __VA_ARGS__ ) : ((void)(0)))
#define LOG_INFO(file, str, ...)      (VERB(INFO)      ? debug("%s:\t" str "\n", file, ## __VA_ARGS__ ) : ((void)(0)))
#define LOG_DEBUG(file, str, ...)     (VERB(DEBUG)     ? debug("%s:\t" str "\n", file, ## __VA_ARGS__ ) : ((void)(0)))
#define LOG_TRACE(file, str, ...)     (VERB(TRACE)     ? debug("%s:\t" str "\n", file, ## __VA_ARGS__ ) : ((void)(0)))
#define LOG_NET_TRACE(file, str, ...) (VERB(NET_TRACE) ? debug("%s:\t" str "\n", file, ## __VA_ARGS__ ) : ((void)(0)))

#define debug_error(f, ...)   LOG_ERROR(__FILE__, f, ## __VA_ARGS__)
#define debug_warning(f, ...) LOG_WARN(__FILE__,  f, ## __VA_ARGS__)
#define debug_notice(f, ...)  LOG_NOTE(__FILE__,  f, ## __VA_ARGS__)
#define debug_info(f, ...)    LOG_INFO(__FILE__,  f, ## __VA_ARGS__)
#define debug_debug(f, ...)   LOG_DEBUG(__FILE__, f, ## __VA_ARGS__)

#endif // DEBUG_H
