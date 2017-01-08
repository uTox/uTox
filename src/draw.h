#ifndef DRAW_H
#define DRAW_H

#include "window.h"

#include <stdbool.h>

/**
 * Sets the target of the next series of drawing commands.
 *
 * Returns true if the window was changed.
 *         false if the window is the same.
 */
bool draw_set_target(UTOX_WINDOW* new_win);


#endif
