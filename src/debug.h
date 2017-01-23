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
    LOG_LVL_TRACE,
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

#define LOG_LVL_ERROR(title, text, ...) debug_error("%s:\t%s\n", title, )


#endif // DEBUG_H
