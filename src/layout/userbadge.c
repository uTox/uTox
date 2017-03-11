#include "userbadge.h"

#include "settings.h"

#include "../avatar.h"
#include "../flist.h"
#include "../macros.h"
#include "../main_native.h"
#include "../self.h"
#include "../tox.h"

#include "../ui/button.h"
#include "../ui/contextmenu.h"
#include "../ui/edit.h"

#include "../main.h" // tox_tread_init


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
        edit_setfocus(&edit_status_msg);
    }
}


#ifdef UNITY
#include "xlib/mmenu.h"
extern bool unity_running;
#endif
static void button_status_on_mup(void) {
    self.status++;
    if (self.status == 3) { // TODO typedef enum
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
        avatar_delete_self();
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

