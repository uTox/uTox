#ifndef UPDATER_H
#define UPDATER_H

#include <stdbool.h>
#include <stdint.h>

/** Returns the new version if there's an update ready.
 *     or zero otherwise
 */
uint32_t updater_check(uint64_t version);

void updater_thread(void *ptr);

// Helper function to reduce needed includes
void updater_start(bool from_startup);

#endif
