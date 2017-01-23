#include "settings.h"

#include "main.h"
#include "logging_native.h"
#include "flist.h"
#include "groups.h"

#include "ui/dropdowns.h"
#include "ui/edits.h"
#include "ui/switches.h"

SETTINGS settings = {
    .curr_version = UTOX_VERSION_NUMBER,
    // .last_version                // included here to match the full struct
    .show_splash = false,

    // Low level settings (network, profile, portable-mode)
    .enable_udp     = true,
    .enable_ipv6    = true,

    .use_proxy      = false,
    .force_proxy    = false,
    .proxy_port     = 0,

    // Tox level settings
    .block_friend_requests  = false,
    .save_encryption        = true,

    // uTox internals
    .auto_update        = false,
    .update_to_develop  = false,
    .send_version       = false,

    // .portable_mode               // included here to match the full struct


    // User interface settings
    .close_to_tray          = false,
    .logging_enabled        = true,
    .audiofilter_enabled    = true,
    .start_in_tray          = false,
    .start_with_system      = false,
    .push_to_talk           = false,
    .audio_preview          = false,
    .video_preview          = false,
    .send_typing_status     = false,
    .use_mini_flist         = false,
    // .inline_video                // included here to match the full struct
    .use_long_time_msg      = true,
    .accept_inline_images   = true,

    // Notifications / Alerts
    .ringtone_enabled       = true,
    .status_notifications   = true,
    .group_notifications    = GNOTIFY_ALWAYS,

    .verbose = 1,

    // .theme                       // included here to match the full struct
    // OS interface settings
    .window_height        = MAIN_HEIGHT,
    .window_width         = MAIN_WIDTH,
    .window_baseline      = 0,

    .window_maximized     = 0,
};

// TODO refactor to match same order in main.h
UTOX_SAVE *config_load(void) {
    UTOX_SAVE *save;
    save = utox_data_load_utox();

    if (!save) {
        debug_notice("unable to load utox_save data\n");
        /* Create and set defaults */
        save              = calloc(1, sizeof(UTOX_SAVE));
        save->enableipv6  = 1;
        save->disableudp  = 0;
        save->proxyenable = 0;

        save->audio_filtering_enabled       = 1;
        save->audible_notifications_enabled = 1;
    }

    if (save->scale > 30) {
        save->scale = 30;
    } else if (save->scale < 5) {
        save->scale = 10;
    }

    if (save->window_width < MAIN_WIDTH) {
        save->window_width = MAIN_WIDTH;
    }
    if (save->window_height < MAIN_HEIGHT) {
        save->window_height = MAIN_HEIGHT;
    }

    dropdown_dpi.selected = dropdown_dpi.over = save->scale - 5;
    dropdown_proxy.selected = dropdown_proxy.over = save->proxyenable <= 2 ? save->proxyenable : 2;

    switch_ipv6.switch_on               = save->enableipv6;
    switch_udp.switch_on                = !save->disableudp;
    switch_save_chat_history.switch_on  = save->logging_enabled;
    switch_mini_contacts.switch_on      = save->use_mini_flist;
    switch_auto_startup.switch_on       = save->auto_startup;
    switch_auto_update.switch_on        = save->auto_update;

    switch_close_to_tray.switch_on = save->close_to_tray;
    switch_start_in_tray.switch_on = save->start_in_tray;

    switch_audible_notifications.switch_on = save->audible_notifications_enabled;
    switch_audio_filtering.switch_on       = save->audio_filtering_enabled;
    switch_push_to_talk.switch_on          = save->push_to_talk;
    switch_status_notifications.switch_on  = save->status_notifications;

    dropdown_theme.selected = dropdown_theme.over = save->theme;

    switch_typing_notes.switch_on = !save->no_typing_notifications;

    flist_set_filter(save->filter); /* roster list filtering */

    /* Network settings */
    settings.enable_ipv6 = save->enableipv6;
    settings.enable_udp  = !save->disableudp;
    settings.use_proxy   = !!save->proxyenable;
    settings.proxy_port  = save->proxy_port;

    if (strlen((char *)save->proxy_ip) <= 256){
        strcpy((char *)proxy_address, (char *)save->proxy_ip);
    }

    edit_proxy_ip.length = strlen((char *)save->proxy_ip);

    strcpy((char *)edit_proxy_ip.data, (char *)save->proxy_ip);

    if (save->proxy_port) {
        edit_proxy_port.length =
            snprintf((char *)edit_proxy_port.data, edit_proxy_port.maxlength + 1, "%u", save->proxy_port);
        if (edit_proxy_port.length >= edit_proxy_port.maxlength + 1) {
            edit_proxy_port.length = edit_proxy_port.maxlength;
        }
    }

    settings.logging_enabled     = save->logging_enabled;
    settings.close_to_tray       = save->close_to_tray;
    settings.start_in_tray       = save->start_in_tray;
    settings.start_with_system   = save->auto_startup;
    settings.ringtone_enabled    = save->audible_notifications_enabled;
    settings.audiofilter_enabled = save->audio_filtering_enabled;
    settings.use_mini_flist      = save->use_mini_flist;

    settings.send_typing_status   = !save->no_typing_notifications;
    settings.group_notifications  = save->group_notifications;
    settings.status_notifications = save->status_notifications;

    settings.window_width  = save->window_width;
    settings.window_height = save->window_height;

    settings.last_version = save->utox_last_version;

    loaded_audio_out_device = save->audio_device_out;
    loaded_audio_in_device  = save->audio_device_in;

    settings.auto_update            = save->auto_update;
    switch_auto_update.switch_on    = save->auto_update;
    settings.update_to_develop      = save->update_to_develop;
    settings.send_version           = save->send_version;


    if (save->push_to_talk) {
        init_ptt();
    }

    return save;
}

// TODO refactor to match order in main.h
void config_save(UTOX_SAVE *save_in) {
    UTOX_SAVE *save = calloc(1, sizeof(UTOX_SAVE) + 256);

    /* Copy the data from the in data to protect the calloc */
    save->window_x      = save_in->window_x;
    save->window_y      = save_in->window_y;
    save->window_width  = save_in->window_width;
    save->window_height = save_in->window_height;

    save->save_version                  = UTOX_SAVE_VERSION;
    save->scale                         = ui_scale - 1;
    save->proxyenable                   = dropdown_proxy.selected;
    save->logging_enabled               = settings.logging_enabled;
    save->close_to_tray                 = settings.close_to_tray;
    save->start_in_tray                 = settings.start_in_tray;
    save->auto_startup                  = settings.start_with_system;
    save->audible_notifications_enabled = settings.ringtone_enabled;
    save->audio_filtering_enabled       = settings.audiofilter_enabled;
    save->push_to_talk                  = settings.push_to_talk;
    save->use_mini_flist                = settings.use_mini_flist;

    save->disableudp              = !settings.enable_udp;
    save->enableipv6              = settings.enable_ipv6;
    save->no_typing_notifications = !settings.send_typing_status;

    save->filter     = flist_get_filter();
    save->proxy_port = settings.proxy_port;

    save->audio_device_in  = dropdown_audio_in.selected;
    save->audio_device_out = dropdown_audio_out.selected;
    save->theme            = settings.theme;

    save->utox_last_version    = settings.curr_version;
    save->group_notifications  = settings.group_notifications;
    save->status_notifications = settings.status_notifications;

    save->auto_update           = settings.auto_update;
    save->update_to_develop     = settings.update_to_develop;
    save->send_version          = settings.send_version;

    memcpy(save->proxy_ip, proxy_address, 256); /* Magic number inside toxcore */

    debug_notice("uTox:\tWriting uTox Save\n");
    utox_data_save_utox(save, sizeof(*save) + 256); /* Magic number inside toxcore */
    free(save);
}
