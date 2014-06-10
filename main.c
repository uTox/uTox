#include "main.h"

#ifdef WIN32
#include "win32/main.c"
#else
#include "xlib/main.c"
#endif
