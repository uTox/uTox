#ifndef MACROS_H
#define MACROS_H

#if (defined __WIN32__ || defined  __ANDROID__) // Windows likes to be broken
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#else
#include <sys/param.h> // Provides MAX/MIN
#endif

// True if x and y are within the supplied rectangle
#define inrect(x, y, rx, ry, width, height) \
    ((x) >= (rx) && (y) >= (ry) && (x) < ((rx) + (width)) && (y) < ((ry) + (height)))

// This is hacky and almost never better, try to use an alternative.
#define strcmp2(x, y) (memcmp(x, y, sizeof(y) - 1))
#define strcpy2(x, y) (memcpy(x, y, sizeof(y) - 1))

// Is the video stream just a selection of the desktop
#define isdesktop(x) ((size_t)(x) == 1)

#define COUNTOF(x) (sizeof(x) / sizeof(*(x)))


// Wrap var in UNUSED(var) to correctly suppress warnings
#ifdef UNUSED
#undef UNUSED
#endif
#ifdef __GNUC__
#define UNUSED(x) UNUSED_##x __attribute__((__unused__))
#elif defined(__LCLINT__)
#define UNUSED(x) /*@unused@*/ x
#else
#define UNUSED(x) x
#endif

#endif
