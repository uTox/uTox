#include "group_invite.h"

#include "create.h"

#include "../flist.h"
#include "../friend.h"
#include "../group_invite.h"
#include "../theme.h"
#include "../tox.h"

#include "../ui/button.h"
#include "../ui/draw.h"
#include "../ui/panel.h"
#include "../ui/svg.h"

static void draw_group_invite(int x, int y, int w, int h);
static void button_group_invite_accept_on_mup(void);
static void button_group_invite_reject_on_mup(void);

BUTTON button_group_invite_accept = {
    .bm_fill     = BM_SBUTTON,
    .button_text = { .i18nal = STR_ADD },
    .on_mup      = button_group_invite_accept_on_mup,
    .update      = button_setcolors_success,
};

BUTTON button_group_invite_reject = {
    .bm_fill     = BM_SBUTTON,
    .button_text = { .i18nal = STR_IGNORE },
    .on_mup      = button_group_invite_reject_on_mup,
    .update      = button_setcolors_success,
};

PANEL panel_group_invite = {
    .disabled = true,
    .drawfunc = draw_group_invite,
    .type = PANEL_NONE,
    .child = (PANEL*[]) {
        (PANEL*)&button_group_invite_accept,
        (PANEL*)&button_group_invite_reject,
        NULL,
    }
};

static void button_group_invite_accept_on_mup(void) {
    const uint8_t invite_id = flist_get_group_invite_id();
    postmessage_toxcore(TOX_GROUP_JOIN, invite_id, 0, NULL);
    panel_group_invite.disabled = true;
}

static void button_group_invite_reject_on_mup(void) {
    const uint8_t invite_id = flist_get_group_invite_id();
    group_invite_reject(invite_id);
    panel_group_invite.disabled = true;
}

static void draw_group_invite(int x, int y, int w, int h) {
    const uint8_t invite_id = flist_get_group_invite_id();
    //const uint32_t friend_id = group_invite_get_friend_id(invite_id);
    //const FRIEND *friend = get_friend(friend_id);

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(x + SCALE(10), SCALE(20), FRIENDREQUEST);

    // if (req->msg && req->length) {
    //     setfont(FONT_TEXT);
    //     utox_draw_text_multiline_within_box(x + SCALE(10), y + SCALE(70), w + x, y, y + h,
    //                                         font_small_lineheight, friend->name,
    //                                         friend->name_length, ~0, ~0, 0,
    //                                         0, true);
    // }
}

void group_invite_draw(void) {
    CREATE_BUTTON(group_invite_accept, 10, MAIN_TOP + 40, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(group_invite_reject, 110, MAIN_TOP + 40, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);
}
