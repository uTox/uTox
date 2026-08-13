#ifndef MOCK_UI_H
#define MOCK_UI_H

#include "../../src/ui.h"
#include "../../src/utox.h"

#include <stdint.h>

/* Draw / font bookkeeping used by widget tests. */
extern int mock_draw_fill_count;
extern int mock_draw_frame_count;
extern int mock_draw_alpha_count;
extern int mock_draw_text_count;
extern int mock_last_font;
extern uint32_t mock_last_color;

extern int mock_thread_count;
extern UTOX_MSG mock_last_utox_msg;

void mock_ui_reset(void);

#endif
