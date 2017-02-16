#ifndef DEBUG_H
#define DEBUG_H

#include "stdlib.h"

#ifndef EXIT_SUCCESS // should be defined in stdlib.h
#define	EXIT_SUCCESS	0	/* Successful exit status.  */
#endif
#ifndef EXIT_FAILURE // should be defined in stdlib.h
#define	EXIT_FAILURE	1	/* Generic failing exit status.  */
#endif
#define	EXIT_MALLOC 	2	/* Malloc failure exit status.  */

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

#define LOG_FATAL_ERR(ex, file, str, ...) (debug("\n\n%-14s:" str "\n\n", file ": ", ## __VA_ARGS__ ) ? exit(ex) : exit(ex))

#define LOG_ERR(file, str, ...)       (VERB(ERROR)     ? debug("%-14s" str "\n", file ": ", ## __VA_ARGS__ ) : ((void)(0)))
#define LOG_WARN(file, str, ...)      (VERB(WARNING)   ? debug("%-14s" str "\n", file ": ", ## __VA_ARGS__ ) : ((void)(0)))
#define LOG_NOTE(file, str, ...)      (VERB(NOTICE)    ? debug("%-14s" str "\n", file ": ", ## __VA_ARGS__ ) : ((void)(0)))
#define LOG_INFO(file, str, ...)      (VERB(INFO)      ? debug("%-14s" str "\n", file ": ", ## __VA_ARGS__ ) : ((void)(0)))
#define LOG_DEBUG(file, str, ...)     (VERB(DEBUG)     ? debug("%-14s" str "\n", file ": ", ## __VA_ARGS__ ) : ((void)(0)))
#define LOG_TRACE(file, str, ...)     (VERB(TRACE)     ? debug("%-14s" str "\n", file ": ", ## __VA_ARGS__ ) : ((void)(0)))
#define LOG_NET_TRACE(file, str, ...) (VERB(NET_TRACE) ? debug("%-14s" str "\n", file ": ", ## __VA_ARGS__ ) : ((void)(0)))

// User requested
#define LOG_NORM(...)       (VERB(OFF) ? debug(__VA_ARGS__ ) : ((void)(0)))


// TODO remove, here for backwards compat
#define debug_warning(f, ...) LOG_WARN(__FILE__ "_OLD",  f, ## __VA_ARGS__)
#define debug_notice(f, ...)  LOG_WARN(__FILE__ "_OLD",  f, ## __VA_ARGS__)
#define debug_info(f, ...)    LOG_WARN(__FILE__ "_OLD",  f, ## __VA_ARGS__)

#endif // DEBUG_H
