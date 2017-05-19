#ifndef LAYOUT_SIDEBAR_H
#define LAYOUT_SIDEBAR_H

#include <stdbool.h>

typedef struct scrollable SCROLLABLE;
extern SCROLLABLE scrollbar_flist;

typedef struct panel PANEL;
extern PANEL panel_side_bar,
             panel_self,

             panel_flist,
             panel_flist_list,

             panel_quick_buttons,
             panel_lower_buttons;

typedef struct button BUTTON;
extern BUTTON button_avatar,
              button_name,
              button_status_msg,
              button_usr_state,

              button_filter_friends,
              button_add_new_contact;

typedef struct edit EDIT;
extern EDIT edit_search;

#endif //LAYOUT_SIDEBAR_H
