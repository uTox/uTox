#ifndef CHRONO_H
#define CHRONO_H

#include <stdint.h>
#include <stdbool.h>

struct chrono_info {
    uint8_t *ptr;
    int step;
    uint32_t interval;
    int ms;
    bool finished;
};

typedef struct chrono_info CHRONO_INFO;

bool chrono_thread_init = false;

/*
 * Starts the chrono thread using the information from info
 * Returns true on success
 * Returns false on failure
 */
bool chrono_start(CHRONO_INFO *info);

/*
 * Ehds the chrono thread
 * Returns true on success
 * Returns false on failure
 */
bool chrono_end(CHRONO_INFO *info);

#endif
