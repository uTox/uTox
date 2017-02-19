#include "debug.h"
#include "settings.h"

#include <assert.h>
#include <malloc.h>
#include <memory.h>
#include <stdarg.h>
#include <stdlib.h>

//
// define some testing helpers
//

#define RUN_TEST(t) printf("\033[01mtest: " #t "...\033[0m\n"); if (t()) { printf("   -> \033[01;32msuccess.\033[0m\n"); } else { printf("   -> \033[01;31mfail.\033[0m\n"); result = 1; }
#define LOG(m, ...) printf("      " m "\n", ## __VA_ARGS__ );
#define FAIL(m, ...) printf("      \033[31m" m "\033[0m\n", ## __VA_ARGS__ ); return false;
#define FAIL_FATAL(m, ...) printf("      \033[31m" m "\033[0m\n", ## __VA_ARGS__ ); exit(1);
