#include "debug.h"

#include "settings.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

int utox_verbosity() {
    return settings.verbose;
}

void debug(const char *fmt, ...){
    if (!settings.debug_file) {
        return;
    }

    va_list list;
    va_start(list, fmt);
    vfprintf(settings.debug_file, fmt, list);
    va_end(list);

    printf("\n\n");

    #ifdef __WIN32__
    fflush(settings.debug_file);
    #endif
}
