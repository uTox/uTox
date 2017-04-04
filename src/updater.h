#ifndef UPDATER_H
#define UPDATER_H

#include <stdbool.h>
#include <stdint.h>

// Returns true if there's a new version.
uint32_t updater_check();

void updater_thread(void *ptr);

// Helper function to reduce needed includes
void updater_start(bool from_startup);

#endif
