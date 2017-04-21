#ifndef CHRONO_H
#define CHRONO_H

#include <stdint.h>
#include <stdbool.h>

struct chrono_info {
    uint8_t *ptr, *target;
    int step;
    uint32_t interval; // in milliseconds
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
 * Ends the chrono thread
 * Returns true on success
 * Returns false on failure
 */
bool chrono_end(CHRONO_INFO *info);

/*
 * Sleep and then
 */
void sleep_callback(uint32_t ms, void func(void *), void *funcargs);

#endif
