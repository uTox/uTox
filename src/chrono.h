#ifndef CHRONO_H
#define CHRONO_H

#include <stdbool.h>
#include <stdint.h>

struct chrono_info {
    int *ptr;
    int *target;
    int step;
    uint32_t interval_ms;
    bool finished;
    void (*callback)(void *);
    void *cb_data;
    void (*sleep_callback)(void *);
    void *sleep_cb_data;
};

typedef struct chrono_info CHRONO_INFO;

extern bool chrono_thread_init;

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
 * Sleep and then run func with funcargs being passed to it
 */
void chrono_callback(uint32_t ms, void func(void *), void *funcargs);

#endif
