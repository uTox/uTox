#ifndef LAYOUT_ROOT_H
#define LAYOUT_ROOT_H

#include "tree.h"

#include "group.h"
#include "friend.h"
#include "settings.h"
#include "side_bar.h"

#include "../ui/panel.h"
#include "../ui/buttons.h"
#include "../ui/draw_helpers.h"

#include <stddef.h>

// clang-format off
/* Root panel, hold all the other panels */

PANEL panel_root = {
    .type = PANEL_NONE,
    .drawfunc = draw_background,
    .disabled = 0,
    .child = (PANEL*[]) { /* Clang warns about this being a temporary array which will be destoryed
                           * at the end of the expression. But because this has always worked in the
                           * past, and because of this comment  http://stackoverflow.com/questions/31212114/clang-complains-pointer-is-initialized-by-a-temporary-array#comment50455257_31212154
                           * I've chosen to ignore this warning. If you're feeling pedantic you can
                           * define and name each array separately and change the PANEL struct. */
        &panel_side_bar,
        &panel_main,
        NULL
    }
},


/* Main panel, holds the overhead/settings, or the friend/group containers */
panel_main = {
    .type = PANEL_NONE,
    .disabled = 0,
    .child = (PANEL*[]) {
        &panel_chat,
        &panel_overhead,
        NULL
    }
},
    /* Chat panel, friend or group, depending on what's selected */
    panel_chat = {
        .type = PANEL_NONE,
        .disabled = 1,
        .child = (PANEL*[]) {
            &panel_group,
            &panel_friend,
            &panel_friend_request,
            NULL
        }
    },

    /* Settings master panel, holds the lower level settings */
    panel_overhead = {
        .type = PANEL_NONE,
        .disabled = 0,
        .child = (PANEL*[]) {
            &panel_splash_page,
            &panel_profile_password,
            &panel_add_friend,
            (PANEL*)&button_notify_create,
            &panel_settings_master,
            NULL
        }
    },
        panel_splash_page = {
            .type = PANEL_NONE,
            .disabled = 1,
            .drawfunc = draw_splash_page,
            .content_scroll = &scrollbar_settings,
            .child = (PANEL*[]) {
                NULL,
            }
        };


#endif
