#include "group_invite.h"

#include "create.h"

#include "../flist.h"
#include "../friend.h"
#include "../group_invite.h"
#include "../macros.h"
#include "../theme.h"
#include "../tox.h"

#include "../ui.h"
#include "../ui/button.h"
#include "../ui/draw.h"
#include "../ui/panel.h"
#include "../ui/svg.h"
#include "../ui/text.h"

static void draw_group_invite(int x, int y, int UNUSED(w), int UNUSED(h));
static void button_group_invite_accept_on_mup(void);
static void button_group_invite_reject_on_mup(void);

void group_invite_draw(void);

BUTTON button_group_invite_accept = {
    .bm_fill     = BM_SBUTTON,
    .button_text = { .i18nal = STR_ACCEPT },
    .on_mup      = button_group_invite_accept_on_mup,
    .update      = button_setcolors_success,
};

BUTTON button_group_invite_reject = {
    .bm_fill     = BM_SBUTTON,
    .button_text = { .i18nal = STR_IGNORE },
    .on_mup      = button_group_invite_reject_on_mup,
    .update      = button_setcolors_danger,
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
    flist_delete_sitem();
    panel_group_invite.disabled = true;
}

static void button_group_invite_reject_on_mup(void) {
    const uint8_t invite_id = flist_get_group_invite_id();
    group_invite_reject(invite_id);
    flist_delete_sitem();
    panel_group_invite.disabled = true;
}

static void draw_group_invite(int x, int y, int UNUSED(w), int UNUSED(h)) {
    const uint8_t invite_id = flist_get_group_invite_id();
    const uint32_t friend_id = group_invite_get_friend_id(invite_id);
    const FRIEND *f = get_friend(friend_id);

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(x + SCALE(10), SCALE(20), GROUP_INVITE);

    setfont(FONT_TEXT);

    const int size = UTOX_FRIEND_NAME_LENGTH(f) + SLEN(GROUP_INVITE_FRIEND);
    char label[size];
    const size_t label_length = snprintf(label, size, S(GROUP_INVITE_FRIEND), UTOX_FRIEND_NAME(f));

    drawtext(x + SCALE(10), y + SCALE(70), label, label_length);
}

void group_invite_draw(void) {
    CREATE_BUTTON(group_invite_accept, -MAIN_TOP, -80, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(group_invite_reject, 10, -80, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);
}
