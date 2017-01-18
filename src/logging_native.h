#ifndef LOGGING_NATIVE_H
#define LOGGING_NATIVE_H

#include <stdio.h>

/* uTox debug levels */
enum {
    VERBOSITY_OFF,
    VERBOSITY_ERROR,
    VERBOSITY_WARNING,
    VERBOSITY_NOTICE,
    VERBOSITY_INFO,
    VERBOSITY_DEBUG,
};

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

#endif // LOGGING_NATIVE_H
