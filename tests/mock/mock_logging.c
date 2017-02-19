#include "../test.h"

// make sure tests are logging everything
int utox_verbosity() { return 100; };
void debug(const char *fmt, ...){
    va_list list;

    va_start(list, fmt);
    printf("      ");
    vprintf(fmt, list);
    va_end(list);
}
