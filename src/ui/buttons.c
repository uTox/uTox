#include "buttons.h"

#include "contextmenu.h"
#include "scrollable.h"
#include "svg.h"

#include "../chatlog.h"
#include "../flist.h"
#include "../friend.h"
#include "../groups.h"
#include "../logging_native.h"
#include "../macros.h"
#include "../main_native.h"
#include "../notify.h"
#include "../screen_grab.h"
#include "../self.h"
#include "../settings.h"
#include "../theme.h"
#include "../tox.h"

#include "../av/audio.h"
#include "../av/utox_av.h"
#include "../av/video.h"
#include "../ui/edits.h"

#include "../layout/tree.h" // TODO remove
#include "../layout/settings.h" // TODO remove

#include "../main.h" // Tox thread globals

#ifdef UNITY
#include "xlib/mmenu.h"
extern bool unity_running;
#endif

// TODO delete button_setcolor_* and move this setting and logic to the struct
/* Quick color change functions */
void button_setcolors_success(BUTTON *b) {
    b->c1  = COLOR_BTN_SUCCESS_BKGRND;
    b->c2  = COLOR_BTN_SUCCESS_BKGRND_HOVER;
    b->c3  = COLOR_BTN_SUCCESS_BKGRND_HOVER;
    b->ct1 = COLOR_BTN_SUCCESS_TEXT;
    b->ct2 = COLOR_BTN_SUCCESS_TEXT_HOVER;
}

void button_setcolors_danger(BUTTON *b) {
    b->c1  = COLOR_BTN_DANGER_BACKGROUND;
    b->c2  = COLOR_BTN_DANGER_BKGRND_HOVER;
    b->c3  = COLOR_BTN_DANGER_BKGRND_HOVER;
    b->ct1 = COLOR_BTN_DANGER_TEXT;
    b->ct2 = COLOR_BTN_DANGER_TEXT_HOVER;
}

void button_setcolors_warning(BUTTON *b) {
    b->c1  = COLOR_BTN_WARNING_BKGRND;
    b->c2  = COLOR_BTN_WARNING_BKGRND_HOVER;
    b->c3  = COLOR_BTN_WARNING_BKGRND_HOVER;
    b->ct1 = COLOR_BTN_WARNING_TEXT;
    b->ct2 = COLOR_BTN_WARNING_TEXT_HOVER;
}

void button_setcolors_disabled(BUTTON *b) {
    b->c1  = COLOR_BTN_DISABLED_BKGRND;
    b->c2  = COLOR_BTN_DISABLED_BKGRND;
    b->c3  = COLOR_BTN_DISABLED_BKGRND;
    b->ct1 = COLOR_BTN_DISABLED_TEXT;
    b->ct2 = COLOR_BTN_DISABLED_TEXT;
}

/* On-press functions followed by the update functions when needed... */
static void button_avatar_on_mup(void) {
    if (tox_thread_init == UTOX_TOX_THREAD_INIT_SUCCESS) {
        openfileavatar();
    }
}

static void button_name_on_mup(void) {
    flist_selectsettings();
    if (tox_thread_init != UTOX_TOX_THREAD_INIT_SUCCESS) {
        // jump to the network settings when unable to create tox instance
        panel_settings_adv.disabled             = false;
        panel_settings_profile.disabled         = true;
        panel_settings_devices.disabled         = true;
        panel_settings_ui.disabled              = true;
        panel_settings_av.disabled              = true;
        panel_settings_notifications.disabled   = true;
    } else {
        panel_settings_profile.disabled         = false;
        panel_settings_devices.disabled         = true;
        panel_settings_ui.disabled              = true;
        panel_settings_av.disabled              = true;
        panel_settings_adv.disabled             = true;
        panel_settings_notifications.disabled   = true;
        edit_setfocus(&edit_name);
    }
}

static void button_statusmsg_on_mup(void) {
    flist_selectsettings();
    if (tox_thread_init != UTOX_TOX_THREAD_INIT_SUCCESS) {
        // jump to the network settings when unable to create tox instance
        panel_settings_profile.disabled         = true;
        panel_settings_devices.disabled         = true;
        panel_settings_ui.disabled              = true;
        panel_settings_av.disabled              = true;
        panel_settings_adv.disabled             = true;
        panel_settings_notifications.disabled   = true;
    } else {
        panel_settings_profile.disabled         = false;
        panel_settings_devices.disabled         = true;
        panel_settings_ui.disabled              = true;
        panel_settings_av.disabled              = true;
        panel_settings_adv.disabled             = true;
        panel_settings_notifications.disabled   = true;
        edit_setfocus(&edit_status);
    }
}

static void button_status_on_mup(void) {
    self.status++;
    if (self.status == 3) {
        self.status = 0;
    }

#ifdef UNITY
    if (unity_running) {
        mm_set_status(self.status);
    }
#endif

    postmessage_toxcore(TOX_SELF_SET_STATE, self.status, 0, NULL);
}

static void contextmenu_avatar_onselect(uint8_t i) {
    if (i == 0) {
        avatar_unset_self();
    }
}

static void button_avatar_onright(void) {
    if (self_has_avatar()) {
        static UTOX_I18N_STR menu[] = { STR_REMOVE };
        contextmenu_new(COUNTOF(menu), menu, contextmenu_avatar_onselect);
    }
}

BUTTON button_avatar = {
    .nodraw = true, .on_mup = button_avatar_on_mup, .onright = button_avatar_onright,
};

BUTTON button_name = {
    .nodraw = true, .on_mup = button_name_on_mup,
};

BUTTON button_status_msg = {
    .nodraw = true, .on_mup = button_statusmsg_on_mup,
};

BUTTON button_usr_state = {
    .nodraw = true,
    .on_mup = button_status_on_mup,
    .tooltip_text = {
        .i18nal = STR_STATUS
    },
};


static void button_filter_friends_on_mup(void) {
    // this only works because right now there are only 2 filters
    // (none or online), basically a bool
    flist_set_filter(!flist_get_filter());
}
BUTTON button_filter_friends = {
    .nodraw = true,
    .on_mup = button_filter_friends_on_mup,
    .tooltip_text = {.i18nal = STR_FILTER_CONTACT_TOGGLE },
};

static void button_group_audio_on_mup(void) {
    GROUPCHAT *g = flist_get_selected()->data;
    if (g->audio_calling) {
        postmessage_toxcore(TOX_GROUP_AUDIO_END, (g - group), 0, NULL);
    } else {
        postmessage_toxcore(TOX_GROUP_AUDIO_START, (g - group), 0, NULL);
    }
}

static void button_group_audio_update(BUTTON *b) {
    GROUPCHAT *g = flist_get_selected()->data;
    if (g->av_group) {
        b->disabled = false;
        if (g->audio_calling) {
            button_setcolors_danger(b);
        } else {
            button_setcolors_success(b);
        }
    } else {
        b->disabled = true;
        button_setcolors_disabled(b);
    }
}

BUTTON button_group_audio = {
    .bm           = BM_LBUTTON,
    .bm2          = BM_CALL,
    .bw           = _BM_LBICON_WIDTH,
    .bh           = _BM_LBICON_HEIGHT,
    .on_mup      = button_group_audio_on_mup,
    .update       = button_group_audio_update,
    .tooltip_text = {.i18nal = STR_GROUPCHAT_JOIN_AUDIO },
};



static void btn_move_window_mdn(void) {
    debug("button move down\n");
    btn_move_window_down = true;
}

static void btn_move_window_mup(void) {
    debug("button move up\n");
    btn_move_window_down = false;
}

static void btn_move_notify_mup(void) {
    debug("button tween\n");
    // window_tween();
}

BUTTON button_move_notify = {
    .nodraw   = false,
    .disabled = false,
    .on_mup   = btn_move_notify_mup,
};


static void btn_notify_create_mup(void) {
    notify_new(NOTIFY_TYPE_MSG);
}

BUTTON button_notify_create = {
    .bm           = BM_SBUTTON,
    .update       = button_setcolors_success,
    .on_mup       = btn_notify_create_mup,
    .button_text  = {.i18nal = STR_SHOW },
    .tooltip_text = {.i18nal = STR_SHOW_UI_PASSWORD },
};
