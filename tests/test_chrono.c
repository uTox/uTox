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

int main(void) {
    int result = 0;
    RUN_TEST(test_chrono_target)
    RUN_TEST(test_chrono_callback)
    return result;
}
