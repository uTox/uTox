// dropdows.h
#include "dropdowns.h"

#include "../av/utox_av.h"
#include "../flist.h"
#include "../friend.h"
#include "../groups.h"
#include "../main.h"


static void dropdown_audio_in_onselect(uint16_t i, const DROPDOWN *dm) {
    DROP_ELEMENT *e      = &((DROP_ELEMENT *)dm->userdata)[i];
    void *        handle = e->handle;
    postmessage_utoxav(UTOXAV_SET_AUDIO_IN, 0, 0, handle);
}

static void dropdown_audio_out_onselect(uint16_t i, const DROPDOWN *dm) {
    DROP_ELEMENT *e      = &((DROP_ELEMENT *)dm->userdata)[i];
    void *        handle = e->handle;
    postmessage_utoxav(UTOXAV_SET_AUDIO_OUT, 0, 0, handle);
}

static void dropdown_video_onselect(uint16_t i, const DROPDOWN *dm) {
    if (i == 1) {
        desktopgrab(1);
    } else {
        postmessage_utoxav(UTOXAV_SET_VIDEO_IN, i, 0, NULL);
    }
}

static void dropdown_dpi_onselect(uint16_t i, const DROPDOWN *UNUSED(dm)) { ui_set_scale(i + 6); }

static void dropdown_language_onselect(uint16_t i, const DROPDOWN *UNUSED(dm)) {
    LANG = (UTOX_LANG)i;
    /* The draw functions need the fonts' and scale to be reset when changing languages. */
    ui_set_scale(ui_scale);
}
static STRING *dropdown_language_ondisplay(uint16_t i, const DROPDOWN *UNUSED(dm)) {
    UTOX_LANG l = (UTOX_LANG)i;
    return SPTRFORLANG(l, STR_LANG_NATIVE_NAME);
}

static void dropdown_proxy_onselect(uint16_t i, const DROPDOWN *UNUSED(dm)) {
    switch (i) {
        case 0: {
            settings.use_proxy   = 0;
            settings.force_proxy = 0;
            break;
        }

        case 1: {
            settings.use_proxy   = 1;
            settings.force_proxy = 0;
            break;
        }

        case 2: {
            settings.use_proxy   = 1;
            settings.force_proxy = 1;
            settings.enable_udp  = 0;
            break;
        }
    }
    memcpy(proxy_address, edit_proxy_ip.data, edit_proxy_ip.length);
    proxy_address[edit_proxy_ip.length] = 0;

    edit_proxy_port.data[edit_proxy_port.length] = 0;
    settings.proxy_port                          = strtol((char *)edit_proxy_port.data, NULL, 0);

    tox_settingschanged();
}

static void dropdown_theme_onselect(uint16_t i, const DROPDOWN *UNUSED(dm)) { theme_load(i); }

static void dropdown_friend_autoaccept_ft_onselect(const uint16_t i, const DROPDOWN *UNUSED(dm)) {
    FRIEND *f        = flist_get_selected()->data;
    f->ft_autoaccept = !!i;
    utox_write_metadata(f);
    debug("Friend %u, is now accepting ft auto %u\n", f->number, i);
}


static void dropdown_notify_groupchats_onselect(const uint16_t i, const DROPDOWN *UNUSED(dm)) {
    GROUPCHAT *g = flist_get_selected()->data;
    g->notify    = i;
    debug("g->notify = %u\n", i);
}

static void dropdown_global_group_notifications_onselect(const uint16_t i, const DROPDOWN *UNUSED(dm)) {
    settings.group_notifications = i;
}

static UTOX_I18N_STR dpidrops[] = {
    STR_DPI_TINY, STR_DPI_060,   STR_DPI_070, STR_DPI_080, STR_DPI_090, STR_DPI_NORMAL, STR_DPI_110,
    STR_DPI_120,  STR_DPI_130,   STR_DPI_140, STR_DPI_BIG, STR_DPI_160, STR_DPI_170,    STR_DPI_180,
    STR_DPI_190,  STR_DPI_LARGE, STR_DPI_210, STR_DPI_220, STR_DPI_230, STR_DPI_240,    STR_DPI_HUGE,
};

static UTOX_I18N_STR proxydrops[] = {
    STR_PROXY_DISABLED, STR_PROXY_FALLBACK, STR_PROXY_ALWAYS_USE,
};

static UTOX_I18N_STR themedrops[] = {
    STR_THEME_DEFAULT, STR_THEME_LIGHT,           STR_THEME_DARK,           STR_THEME_HIGHCONTRAST, STR_THEME_CUSTOM,
    STR_THEME_ZENBURN, STR_THEME_SOLARIZED_LIGHT, STR_THEME_SOLARIZED_DARK,
};

static UTOX_I18N_STR noyesdrops[] = { STR_NO, STR_YES };

static UTOX_I18N_STR notifydrops[] = {
    STR_GROUP_NOTIFICATIONS_OFF, STR_GROUP_NOTIFICATIONS_MENTION, STR_GROUP_NOTIFICATIONS_ON,
};

DROPDOWN dropdown_audio_in = {.ondisplay = list_dropdown_ondisplay, .onselect = dropdown_audio_in_onselect },

         dropdown_audio_out = {.ondisplay = list_dropdown_ondisplay, .onselect = dropdown_audio_out_onselect },

         dropdown_video =
             {
               .ondisplay = list_dropdown_ondisplay, .onselect = dropdown_video_onselect,
             },

         dropdown_dpi = {.ondisplay = simple_dropdown_ondisplay,
                         .onselect  = dropdown_dpi_onselect,
                         .dropcount = countof(dpidrops),
                         .userdata  = dpidrops },

         dropdown_language =
             {
               .ondisplay = dropdown_language_ondisplay, .onselect = dropdown_language_onselect, .dropcount = NUM_LANGS,
             },

         dropdown_proxy = {.ondisplay = simple_dropdown_ondisplay,
                           .onselect  = dropdown_proxy_onselect,
                           .dropcount = countof(proxydrops),
                           .userdata  = proxydrops },

         dropdown_theme = {.ondisplay = simple_dropdown_ondisplay,
                           .onselect  = dropdown_theme_onselect,
                           .dropcount = countof(themedrops),
                           .userdata  = themedrops },

         dropdown_friend_autoaccept_ft = {.ondisplay = simple_dropdown_ondisplay,
                                          .onselect  = dropdown_friend_autoaccept_ft_onselect,
                                          .dropcount = countof(noyesdrops),
                                          .userdata  = noyesdrops },

         dropdown_notify_groupchats = {.ondisplay = simple_dropdown_ondisplay,
                                       .onselect  = dropdown_notify_groupchats_onselect,
                                       .dropcount = countof(notifydrops),
                                       .userdata  = notifydrops },

         dropdown_global_group_notifications = {.ondisplay = simple_dropdown_ondisplay,
                                                .onselect  = dropdown_global_group_notifications_onselect,
                                                .dropcount = countof(notifydrops),
                                                .userdata  = notifydrops };
