#include "debug.h"

#include "settings.h"

#include <stdio.h>
#include <stdarg.h>

int utox_verbosity() {
    return settings.verbose;
}

#ifndef __ANDROID__ // Android needs to provide it's own logging functions

void debug(const char *fmt, ...){
    va_list list;

    va_start(list, fmt);
    vfprintf(settings.debug_file, fmt, list);
    va_end(list);

    #ifdef __WIN32__
    fflush(settings.debug_file);
    #endif
}

#endif
