#include "../src/chrono.c"

#include "test.h"

#include <time.h>
#include <stdint.h>

/*
START_TEST (test_chrono_finished)
{
    CHRONO_INFO info;

    info.ptr = 0;
    info.step = 5;
    info.interval_ms = 5;
    info.finished = false;

    chrono_start(&info);

    yieldcpu(30);

    chrono_end(&info);

    ck_assert_msg((intptr_t)info.ptr == 30, "Expected 30 got: %u", info.ptr);
}
END_TEST
*/

void thread_callback(void *args) {
    *(bool *)args = true;
}

START_TEST(test_chrono_target)
{
    /*
     * Chrono info should be mallocated in real code.
     * This function can't exit until the thread exits so it is safe
     * to use the stack.
     */
    CHRONO_INFO info;

    memset(&info, 0, sizeof(info));

    bool finished = false;
    int target = 30, ptr = 0;

    info.ptr = &ptr;
    info.step = 5;
    info.interval_ms = 5;
    info.finished = false;
    info.target = &target;
    info.callback = thread_callback;
    info.cb_data = &finished;

    chrono_start(&info);

    yieldcpu(30); // allow thread to run and exit

    while (!finished) {
        yieldcpu(1);
    }

    ck_assert_msg(*info.ptr == 30, "Expected 30 got: %u", info.ptr);
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
