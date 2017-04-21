#include "chrono.h"
#include "debug.h"
#include "macros.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "native/thread.h"

static void chrono_thread(void *args) {
    LOG_INFO("Chono", "Thread starting");
    chrono_thread_init = true;
    CHRONO_INFO *info = args;
    while(!info->finished) {
        (*info).ptr += info->step;
        yieldcpu(info->interval);
    }
    chrono_thread_init = false;
    LOG_INFO("Chrono", "Thread exited cleanly");
}

bool chrono_start(CHRONO_INFO *info) {
    if (!info) {
        LOG_ERR("Chrono", "Chrono info structure is null.");
        return false;
    }

    thread(chrono_thread, info);
    return true;
}

bool chrono_end(CHRONO_INFO *info) {
    if (!info) {
        LOG_ERR("Chrono", "Chrono info is null");
        return false;
    }

    (*info).finished = true;

    while (chrono_thread_init) { //wait for thread to die
        yieldcpu(1);
    }

    return true;
}
