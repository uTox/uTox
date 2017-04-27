#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdlib.h>

#ifndef EXIT_SUCCESS // should be defined in stdlib.h
#define EXIT_SUCCESS 0 /* Successful exit status. */
#endif
#ifndef EXIT_FAILURE // should be defined in stdlib.h
#define EXIT_FAILURE 1 /* Generic failing exit status. */
#endif
#define EXIT_MALLOC  2 /* Malloc failure exit status. */

/* uTox debug levels */
typedef enum {
    LOG_LVL_OFF,
    LOG_LVL_FATAL,
    LOG_LVL_ERROR,      // This is really bad
    LOG_LVL_WARNING,    // This is kinda bad
    LOG_LVL_NOTICE,     // This is just something we should know about
    LOG_LVL_INFO,       // This is what's happening
    LOG_LVL_DEBUG,      // Something is broken, whats' happening everywhere
    LOG_LVL_TRACE,      // I'm not kidding anymore... WHAT IS BROKEN?
    LOG_LVL_NET_TRACE,  // OH, it's Toxcore that's broken? Whew!
} LOG_LVL;

// returns current logging verbosity
int utox_verbosity();

// define debugging macros in a platform specific way

#ifdef __ANDROID__
#include "android/logging.h"
#else
void debug(const char *fmt, ...);
#endif

#define VERB(x) (utox_verbosity() >= LOG_LVL_##x)

#define LOG_FATAL_ERR(ex, ...) \
    printf("%s:%d\n\t", __FILE__, __LINE__); debug(__VA_ARGS__); exit(ex)

#define LOG_ERR(...)       (VERB(ERROR) ? \
    printf("%s:%d\n\t", __FILE__, __LINE__), debug(__VA_ARGS__) : ((void)(0)))
#define LOG_WARN(...)      (VERB(WARNING) ? \
    printf("%s:%d\n\t", __FILE__, __LINE__), debug(__VA_ARGS__) : ((void)(0)))
#define LOG_NOTE(...)      (VERB(NOTICE) ? \
    printf("%s:%d\n\t", __FILE__, __LINE__), debug(__VA_ARGS__) : ((void)(0)))
#define LOG_INFO(...)      (VERB(INFO) ? \
    printf("%s:%d\n\t", __FILE__, __LINE__), debug(__VA_ARGS__) : ((void)(0)))
#define LOG_DEBUG(...)     (VERB(DEBUG) ? \
    printf("%s:%d\n\t", __FILE__, __LINE__), debug(__VA_ARGS__) : ((void)(0)))
#define LOG_TRACE(...)     (VERB(TRACE) ? \
    printf("%s:%d\n\t", __FILE__, __LINE__), debug(__VA_ARGS__) : ((void)(0)))
#define LOG_NET_TRACE(...) (VERB(NET_TRACE) ? \
    printf("%s:%d\n\t", __FILE__, __LINE__), debug(__VA_ARGS__) : ((void)(0)))

// User requested
#define LOG_NORM(...)       (VERB(OFF) ? debug(__VA_ARGS__ ) : ((void)(0)))

#endif // DEBUG_H
