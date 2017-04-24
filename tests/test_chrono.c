#include "../src/chrono.c"

#include "test.h"

#include <time.h>
#include <stdlib.h>
#include <inttypes.h>

/*
START_TEST (test_chrono_finished)
{
    CHRONO_INFO *info = malloc(sizeof(CHRONO_INFO));
    if (!info) {
        return;
    }

    info->ptr = 0;
    info->step = 5;
    info->interval_ms = 5;
    info->finished = false;

    chrono_start(info);

    yieldcpu(30);

    chrono_end(info);

    ck_assert_msg((intptr_t)info->ptr == 30, "Expected 30 got: %u", info->ptr);

    free(info);
}
END_TEST
*/

START_TEST(test_chrono_target)
{
    CHRONO_INFO *info = malloc(sizeof(CHRONO_INFO));
    if (!info) {
        return;
    }

    info->ptr = 0;
    info->step = 5;
    info->interval_ms = 5;
    info->finished = false;
    info->target = (uint8_t *)30;

    chrono_start(info);

    yieldcpu(31); // allow thread to run and exit
                  // 30 to get to the target and 1 to exit

    ck_assert_msg((intptr_t)info->ptr == 30, "Expected 30 got: %u", info->ptr);

    free(info);

}
END_TEST

void callback(void *arg) {
    *(int *)arg = 10;
}

START_TEST(test_chrono_callback)
{
    int arg = 0;
    chrono_callback(1, callback, &arg);
    ck_assert_msg((intptr_t)arg == 10, "Expected callback_arg to be 10 got: %d", arg);
}
END_TEST

static Suite *suite(void)
{
    Suite *s = suite_create("Chrono");

    //MK_TEST_CASE(chrono_finished); // reenable when the finished field of the chrono info struct is used
    MK_TEST_CASE(chrono_target);
    MK_TEST_CASE(chrono_callback)

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
