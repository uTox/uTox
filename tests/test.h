#include "../src/debug.h"
#include "../src/settings.h"

#include <assert.h>
#include <malloc.h>
#include <memory.h>
#include <stdarg.h>
#include <stdlib.h>

#include <assert.h>
#include <check.h>

#define MK_TEST_CASE(TRGT)                   \
    TCase *case_##TRGT = tcase_create(#TRGT); \
    tcase_add_test(case_##TRGT, test_##TRGT); \
    suite_add_tcase(s, case_##TRGT);

//
// define some testing helpers
//

// run a test method and print its name to create a nicely readable testing output
#define RUN_TEST(t) printf("\033[01mtest: " #t "...\033[0m\n"); if (t()) { printf("   -> \033[01;32msuccess.\033[0m\n"); } else { printf("   -> \033[01;31mfail.\033[0m\n"); result = 1; }
// print a message
#define LOG(m, ...) printf("      " m "\n", ## __VA_ARGS__ );
// print message and return false (i.e. fail current test method)
#define FAIL(m, ...) printf("      \033[31m" m "\033[0m\n", ## __VA_ARGS__ ); return false;
// print message and exit unsuccessfully
#define FAIL_FATAL(m, ...) printf("      \033[31m" m "\033[0m\n", ## __VA_ARGS__ ); exit(1);
