#include "../src/chrono.c"

#include "test.h"

#include <time.h>
#include <stdlib.h>
#include <inttypes.h>

START_TEST (test_chrono)
{
    CHRONO_INFO *info = malloc(sizeof(CHRONO_INFO*));
    if (!info) {
        return;
    }

    info->ptr = 0;
    info->step = 5;
    info->interval = 5;
    info->finished = false;

    chrono_start(info);

    yieldcpu(30);

    chrono_end(info);

    ck_assert_msg((intptr_t)info->ptr == 30, "Expected 30 got: %u", info->ptr);

    free(info);
}
END_TEST

static Suite *suite(void)
{
    Suite *s = suite_create("Chrono");

    MK_TEST_CASE(chrono);

    return s;
}

int main(int argc, char *argv[])
{
    srand((unsigned int) time(NULL));

    Suite *run = suite();
    SRunner *test_runner = srunner_create(run);

    int number_failed = 0;
    srunner_run_all(test_runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(test_runner);

    srunner_free(test_runner);

    return number_failed;
}
