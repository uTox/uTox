// Uncomment when the native function cleanup is done.
// #if defined(NATIVE_MAIN_H)
// #error "The main function should only ever be included once."
// #endif

#ifndef NATIVE_MAIN_H
#define NATIVE_MAIN_H

#if defined __WIN32__ || defined _WIN32 || defined __CYGWIN__
#include "../windows/main.h"
#elif defined __ANDROID__
#include "../android/main.h"
#elif defined __OBJC__
#include "../cocoa/main.h"
#else
#include "../xlib/main.h"
#endif

#endif
