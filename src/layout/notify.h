#ifndef LAYOUT_NOTIFY_H
#define LAYOUT_NOTIFY_H

#include <stdbool.h>

typedef struct panel PANEL;
extern PANEL panel_notify_generic;

typedef struct button BUTTON;
extern BUTTON button_notify_one,
              button_notify_two,
              button_notify_three,
              button_move_notify,
              button_notify_create;


// TODO, no good capin'
bool btn_move_window_down;


#endif // LAYOUT_NOTIFY_H
