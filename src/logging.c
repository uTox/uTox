#include "debug.h"

#include "settings.h"

#include <stdio.h>
#include <stdarg.h>

int utox_verbosity() {
    return settings.verbose;
}

void debug(const char *fmt, ...){
    va_list list;
    FILE *file = stderr;

    if (settings.debug_file) {
        file = settings.debug_file;
    }

    va_start(list, fmt);
    vfprintf(file, fmt, list);
    va_end(list);

    #ifdef __WIN32__
    fflush(file);
    #endif
}
