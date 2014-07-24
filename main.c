#include "main.h"

#ifdef __WIN32__
#include "win32/main.c"
#else
#ifdef __ANDROID__
#include "android/main.c"
#else
#include "xlib/main.c"
#endif
#endif
