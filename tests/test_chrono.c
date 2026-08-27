#include "../src/chrono.c"

#include "test.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

void thread_callback(void *args) {
    *(bool *)args = true;
}

bool test_chrono_target(void) {
    /*
     * Chrono info should be mallocated in real code.
     * This function can't exit until the thread exits so it is safe
     * to use the stack.
     */
    CHRONO_INFO info;
    bool finished = false;

    info.ptr = 0;
    info.step = 5;
    info.interval_ms = 5;
    info.finished = false;
    info.target = (uint8_t *)30;
    info.callback = thread_callback;
    info.cb_data = &finished;

    chrono_start(&info);

    yieldcpu(30);

    while (!finished) {
        yieldcpu(1);
    }

    if ((intptr_t)info.ptr != 30) {
        FAIL("Expected 30 got: %u", (unsigned)(uintptr_t)info.ptr);
    }
    return true;
}

void callback(void *arg) {
    *(int *)arg = 10;
}

bool test_chrono_callback(void) {
    int arg = 0;
    chrono_callback(1, callback, &arg);
    if (arg != 10) {
        FAIL("Expected callback_arg to be 10 got: %d", arg);
    }
    return true;
}

bool test_chrono_null_and_end(void) {
    settings.verbose = LOG_LVL_TRACE;
    if (chrono_start(NULL)) {
        FAIL("chrono_start(NULL) should fail");
    }
    if (chrono_end(NULL)) {
        FAIL("chrono_end(NULL) should fail");
    }

    /* Quiet run so LOG_INFO in the worker takes the VERB() false side. */
    settings.verbose = LOG_LVL_OFF;
    CHRONO_INFO quiet;
    quiet.ptr         = 0;
    quiet.step        = 1;
    quiet.interval_ms = 1;
    quiet.finished    = false;
    quiet.target      = (uint8_t *)2;
    quiet.callback    = NULL;
    quiet.cb_data     = NULL;
    if (!chrono_start(&quiet)) {
        FAIL("quiet chrono_start");
    }
    int qspins = 0;
    while (!chrono_thread_init && (intptr_t)quiet.ptr == 0 && qspins++ < 1000) {
        yieldcpu(1);
    }
    if (!chrono_end(&quiet)) {
        FAIL("quiet chrono_end");
    }

    settings.verbose = LOG_LVL_TRACE;

    CHRONO_INFO info;
    info.ptr         = 0;
    info.step        = 1;
    info.interval_ms = 1;
    info.finished    = false;
    info.target      = (uint8_t *)8;
    info.callback    = NULL;
    info.cb_data     = NULL;

    if (!chrono_start(&info)) {
        FAIL("chrono_start should succeed");
    }
    /* Don't call chrono_end before the worker sets chrono_thread_init — otherwise
     * we return while the thread still owns this stack frame. */
    int spins = 0;
    while (!chrono_thread_init && (intptr_t)info.ptr == 0 && spins++ < 1000) {
        yieldcpu(1);
    }
    if (!chrono_end(&info)) {
        FAIL("chrono_end while running should succeed");
    }
    if (!info.finished) {
        FAIL("chrono_end should set finished");
    }
    if ((intptr_t)info.ptr != 8) {
        FAIL("thread should still reach target, got %d", (int)(intptr_t)info.ptr);
    }
    return true;
}

int main(void) {
    int result = 0;
    RUN_TEST(test_chrono_target)
    RUN_TEST(test_chrono_callback)
    RUN_TEST(test_chrono_null_and_end)
    return result;
}
