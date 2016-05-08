#include "../main.h"

#ifdef UNITY
#include "xlib/mmenu.h"
extern _Bool unity_running;
#endif

/* Function which removes an entry from the messaging menu
 * Is called by xlib/{list,event}.c and takes in parameter a friend ID
 */
void mm_rm_entry(uint8_t *f_id);

/* Function which sets the user status in the messaging menu
 * Is called by ui_buttons.h
 */
 void mm_set_status(int status);
