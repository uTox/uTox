#include "main.h"

#ifdef __WIN32__
#include "win32/main.c"
#else
#include "xlib/main.c"
#endif
