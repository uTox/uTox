#ifndef MACROS_H
#define MACROS_H

#define inrect(x, y, rx, ry, width, height) \
    ((x) >= (rx) && (y) >= (ry) && (x) < ((rx) + (width)) && (y) < ((ry) + (height)))

#define strcmp2(x, y) (memcmp(x, y, sizeof(y) - 1))
#define strcpy2(x, y) (memcpy(x, y, sizeof(y) - 1))

#define isdesktop(x) ((size_t)(x) == 1)

#define countof(x) (sizeof(x) / sizeof(*(x)))

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
