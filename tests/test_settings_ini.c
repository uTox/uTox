#include "test.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifndef _WIN32
#include <sys/stat.h>
#endif

#include "../src/settings.c"

/* Headless stand-ins for widgets that config_load/config_save sync. */
DROPDOWN dropdown_language;
DROPDOWN dropdown_theme;
DROPDOWN dropdown_dpi;
DROPDOWN dropdown_audio_in;
DROPDOWN dropdown_audio_out;
DROPDOWN dropdown_global_group_notifications;

UISWITCH switch_save_chat_history;
UISWITCH switch_close_to_tray;
UISWITCH switch_start_in_tray;
UISWITCH switch_auto_startup;
UISWITCH switch_mini_contacts;
UISWITCH switch_magic_sidebar;
UISWITCH switch_ipv6;
UISWITCH switch_udp;
UISWITCH switch_proxy;
UISWITCH switch_proxy_force;
UISWITCH switch_push_to_talk;
UISWITCH switch_audio_filtering;
UISWITCH switch_audible_notifications;
UISWITCH switch_status_notifications;
UISWITCH switch_typing_notes;

static char edit_video_fps_data[8];
static char edit_proxy_ip_data[256];
static char edit_proxy_port_data[8];

EDIT edit_video_fps = {
    .data      = edit_video_fps_data,
    .data_size = sizeof edit_video_fps_data,
};
EDIT edit_proxy_ip = {
    .data      = edit_proxy_ip_data,
    .data_size = sizeof edit_proxy_ip_data,
};
EDIT edit_proxy_port = {
    .data      = edit_proxy_port_data,
    .data_size = sizeof edit_proxy_port_data,
};

uint8_t flist_get_filter(void) {
    return 0;
}

void flist_set_filter(uint8_t filter) {
    (void)filter;
}

void ui_set_scale(uint8_t scale) {
    (void)scale;
}

void init_ptt(void) {
}

static void unlink_ini(void) {
    unlink("./tox/utox_save.ini");
    rmdir("./tox/utox_save.ini"); /* leftover dir from a previous failed run */
}

bool test_settings_ini_roundtrip(void) {
    settings.portable_mode   = true;
    settings.logging_enabled = false;
    settings.close_to_tray   = true;
    settings.theme           = 3;
    settings.scale           = 12;
    settings.window_width    = 900;
    settings.window_height   = 700;
    settings.enableipv6      = false;
    settings.proxy_port      = 8080;
    memset(settings.proxy_ip, 0, sizeof(settings.proxy_ip));
    strncpy((char *)settings.proxy_ip, "127.0.0.1", sizeof(settings.proxy_ip) - 1);

    unlink_ini();
    if (!utox_save_config()) {
        FAIL("utox_save_config failed");
    }

    settings.logging_enabled = true;
    settings.close_to_tray   = false;
    settings.theme           = UINT32_MAX; /* only applied from INI when unset */
    settings.scale           = 5;
    settings.window_width    = MAIN_WIDTH;
    settings.window_height   = MAIN_HEIGHT;
    settings.enableipv6      = true;
    settings.proxy_port      = 0;
    memset(settings.proxy_ip, 0, sizeof(settings.proxy_ip));

    if (!utox_load_config()) {
        unlink_ini();
        FAIL("utox_load_config failed");
    }

    if (settings.logging_enabled) {
        unlink_ini();
        FAIL("logging_enabled not restored");
    }
    if (!settings.close_to_tray) {
        unlink_ini();
        FAIL("close_to_tray not restored");
    }
    if (settings.theme != 3) {
        unlink_ini();
        FAIL("theme not restored got %u", settings.theme);
    }
    if (settings.scale != 12) {
        unlink_ini();
        FAIL("scale not restored got %u", settings.scale);
    }
    if (settings.window_width != 900 || settings.window_height != 700) {
        unlink_ini();
        FAIL("window size not restored %ux%u", settings.window_width, settings.window_height);
    }
    if (settings.enableipv6) {
        unlink_ini();
        FAIL("enableipv6 not restored");
    }
    if (settings.proxy_port != 8080) {
        unlink_ini();
        FAIL("proxy_port not restored");
    }
    if (strcmp((char *)settings.proxy_ip, "127.0.0.1") != 0) {
        unlink_ini();
        FAIL("proxy_ip not restored: %s", settings.proxy_ip);
    }

    unlink_ini();
    return true;
}

bool test_settings_ini_unknown_keys(void) {
    settings.portable_mode = true;
    unlink_ini();

    if (!native_create_dir((uint8_t *)"./tox/")) {
        /* may already exist */
    }

    FILE *fp = fopen("./tox/utox_save.ini", "wb");
    if (!fp) {
        FAIL("unable to write utox_save.ini");
    }
    fputs("[interface]\n", fp);
    fputs("theme = 2\n", fp);
    fputs("totally_unknown_key = true\n", fp);
    fputs("[not_a_section]\n", fp);
    fputs("foo = bar\n", fp);
    fclose(fp);

    settings.theme = UINT32_MAX;
    if (!utox_load_config()) {
        unlink_ini();
        FAIL("load with unknown keys failed");
    }
    if (settings.theme != 2) {
        unlink_ini();
        FAIL("theme should load despite unknown keys, got %u", settings.theme);
    }

    unlink_ini();
    return true;
}

bool test_config_load_defaults_and_clamps(void) {
    unlink_ini();
    settings.portable_mode = true;
    settings.theme         = UINT32_MAX;
    settings.scale         = 99;
    settings.window_width  = 10;
    settings.window_height = 10;
    settings.enableipv6    = false;
    settings.video_fps     = 0;
    settings.push_to_talk  = true;

    config_load();

    if (settings.theme != 0) {
        unlink_ini();
        FAIL("missing ini should default theme, got %u", settings.theme);
    }
    if (settings.window_width < MAIN_WIDTH || settings.window_height < MAIN_HEIGHT) {
        unlink_ini();
        FAIL("window size should clamp to MAIN_*");
    }
    if (settings.scale != 30) {
        unlink_ini();
        FAIL("scale 99 should clamp to 30, got %u", settings.scale);
    }
    if (settings.video_fps != DEFAULT_FPS) {
        unlink_ini();
        FAIL("video_fps 0 should become DEFAULT_FPS");
    }
    if (dropdown_dpi.selected != settings.scale - 5) {
        unlink_ini();
        FAIL("dpi dropdown not synced");
    }

    /* Low scale clamp via public config_load after writing a tiny value. */
    settings.scale = 1;
    if (!utox_save_config()) {
        unlink_ini();
        FAIL("save tiny scale");
    }
    settings.scale = 12;
    config_load();
    if (settings.scale != 10) {
        unlink_ini();
        FAIL("scale < 5 should become 10, got %u", settings.scale);
    }

    unlink_ini();
    return true;
}

bool test_config_all_sections(void) {
    settings.portable_mode = true;
    unlink_ini();
    native_create_dir((uint8_t *)"./tox/");

    FILE *fp = fopen("./tox/utox_save.ini", "wb");
    if (!fp) {
        FAIL("write full utox_save.ini");
    }
    fputs("[general]\n", fp);
    fputs("save_version = 3\n", fp);
    fputs("utox_last_version = 18\n", fp);
    fputs("[interface]\n", fp);
    fputs("language = 2\n", fp);
    fputs("window_x = 11\n", fp);
    fputs("window_y = 22\n", fp);
    fputs("window_width = 800\n", fp);
    fputs("window_height = 600\n", fp);
    fputs("theme = 4\n", fp);
    fputs("scale = 15\n", fp);
    fputs("logging_enabled = false\n", fp);
    fputs("close_to_tray = true\n", fp);
    fputs("start_in_tray = true\n", fp);
    fputs("auto_startup = true\n", fp);
    fputs("use_mini_flist = true\n", fp);
    fputs("filter = true\n", fp);
    fputs("magic_flist_enabled = true\n", fp);
    fputs("use_long_time_msg = false\n", fp);
    fputs("[av]\n", fp);
    fputs("push_to_talk = true\n", fp);
    fputs("audio_filtering_enabled = false\n", fp);
    fputs("audio_device_in = 2\n", fp);
    fputs("audio_device_out = 3\n", fp);
    fputs("video_fps = 15\n", fp);
    fputs("[notifications]\n", fp);
    fputs("audible_notifications_enabled = false\n", fp);
    fputs("status_notifications = false\n", fp);
    fputs("no_typing_notifications = false\n", fp);
    fputs("group_notifications = 1\n", fp);
    fputs("[advanced]\n", fp);
    fputs("enableipv6 = false\n", fp);
    fputs("disableudp = true\n", fp);
    fputs("proxyenable = true\n", fp);
    fputs("proxy_port = 9050\n", fp);
    fputs("proxy_ip = 10.0.0.1\n", fp);
    fputs("force_proxy = true\n", fp);
    fputs("block_friend_requests = true\n", fp);
    fclose(fp);

    settings.theme = UINT32_MAX;
    settings.video_fps = 0;
    memset(settings.proxy_ip, 0, sizeof(settings.proxy_ip));
    config_load();

    if (settings.save_version != 3 || settings.utox_last_version != 18) {
        unlink_ini();
        FAIL("general section");
    }
    if (settings.language != 2 || settings.window_x != 11 || settings.window_y != 22) {
        unlink_ini();
        FAIL("interface geometry/language");
    }
    if (settings.logging_enabled || !settings.close_to_tray || !settings.use_mini_flist
        || !settings.magic_flist_enabled || settings.use_long_time_msg) {
        unlink_ini();
        FAIL("interface flags");
    }
    if (!settings.push_to_talk || settings.audio_filtering_enabled || settings.video_fps != 15) {
        unlink_ini();
        FAIL("av section fps=%u", settings.video_fps);
    }
    if (settings.audible_notifications_enabled || settings.status_notifications
        || settings.no_typing_notifications || settings.group_notifications != 1) {
        unlink_ini();
        FAIL("notifications section");
    }
    if (settings.enableipv6 || !settings.disableudp || !settings.proxyenable
        || settings.proxy_port != 9050 || !settings.force_proxy || !settings.block_friend_requests) {
        unlink_ini();
        FAIL("advanced flags");
    }
    if (strcmp((char *)settings.proxy_ip, "10.0.0.1") != 0) {
        unlink_ini();
        FAIL("proxy_ip %s", settings.proxy_ip);
    }
    if (edit_video_fps.length == 0 || edit_proxy_port.length == 0) {
        unlink_ini();
        FAIL("edit widgets not filled");
    }

    dropdown_audio_in.selected  = 2;
    dropdown_audio_out.selected = 3;
    config_save();

    /* Invalid fps is ignored (keeps previous). */
    fp = fopen("./tox/utox_save.ini", "ab");
    if (!fp) {
        unlink_ini();
        FAIL("append invalid fps");
    }
    fputs("video_fps = 0\n", fp);
    fputs("video_fps = nope\n", fp);
    fclose(fp);
    const uint16_t fps_before = settings.video_fps;
    if (!utox_load_config()) {
        unlink_ini();
        FAIL("reload after invalid fps");
    }
    if (settings.video_fps != fps_before) {
        unlink_ini();
        FAIL("invalid fps should be ignored, got %u", settings.video_fps);
    }

    unlink_ini();
    return true;
}

bool test_config_edge_cases(void) {
    settings.portable_mode = true;
    settings.verbose       = LOG_LVL_TRACE;
    unlink_ini();
    native_create_dir((uint8_t *)"./tox/");

    FILE *fp = fopen("./tox/utox_save.ini", "wb");
    if (!fp) {
        FAIL("write edge utox_save.ini");
    }
    fputs("[general]\n", fp);
    fputs("not_a_key = 1\n", fp);
    fputs("[interface]\n", fp);
    fputs("theme = 4\n", fp);
    fputs("scale = 5\n", fp);
    fputs("window_width = 800\n", fp);
    fputs("window_height = 600\n", fp);
    fputs("[av]\n", fp);
    fputs("video_fps = 256\n", fp);
    fputs("video_fps = 1\n", fp);
    fputs("push_to_talk = false\n", fp);
    fputs("[notifications]\n", fp);
    fputs("group_notifications = 2\n", fp);
    fputs("[advanced]\n", fp);
    fputs("proxy_port = 0\n", fp);
    fputs("[unknown_section]\n", fp);
    fputs("whatever = true\n", fp);
    fclose(fp);

    settings.theme      = 2; /* already set: ini must not override */
    settings.video_fps  = 30;
    settings.proxy_port = 0;
    settings.push_to_talk = true;
    edit_proxy_port.length = 99;
    config_load();

    if (settings.theme != 2) {
        unlink_ini();
        FAIL("cmdline theme should win, got %u", settings.theme);
    }
    if (settings.scale != 5) {
        unlink_ini();
        FAIL("scale 5 is a valid lower bound, got %u", settings.scale);
    }
    if (settings.video_fps != 1) {
        unlink_ini();
        FAIL("video_fps 256 ignored then 1 applied, got %u", settings.video_fps);
    }
    if (settings.proxy_port != 0 || edit_proxy_port.length != 99) {
        unlink_ini();
        FAIL("proxy_port 0 should skip filling the edit");
    }
    if (settings.group_notifications != 2) {
        unlink_ini();
        FAIL("group_notifications");
    }

    /* Valid upper scale bound. */
    fp = fopen("./tox/utox_save.ini", "wb");
    if (!fp) {
        unlink_ini();
        FAIL("write scale 30");
    }
    fputs("[interface]\nscale = 30\n", fp);
    fclose(fp);
    settings.scale = 12;
    if (!utox_load_config()) {
        unlink_ini();
        FAIL("reload scale 30");
    }
    config_load();
    if (settings.scale != 30) {
        unlink_ini();
        FAIL("scale 30 should stay 30, got %u", settings.scale);
    }

    unlink_ini();

#ifndef _WIN32
    {
        FILE *seed = fopen("./tox/utox_save.ini", "wb");
        if (!seed) {
            FAIL("seed ini for chmod");
        }
        fputs("[general]\nsave_version = 1\n", seed);
        fclose(seed);
        if (chmod("./tox/utox_save.ini", 0444) == 0) {
            config_save(); /* ini_put* fail → LOG_ERR on each WRITE */
            chmod("./tox/utox_save.ini", 0644);
        }
        unlink_ini();
    }
#endif
    return true;
}

int main(void) {
    int result = 0;
    settings.portable_mode = true;
    settings.verbose       = LOG_LVL_ERROR;
    unlink("./tox"); /* leftover file from a crashed run; no-op if directory */
    rename("./tox.bak", "./tox");
    unlink_ini();
    native_create_dir((uint8_t *)"./tox/");
    RUN_TEST(test_settings_ini_roundtrip);
    RUN_TEST(test_settings_ini_unknown_keys);
    RUN_TEST(test_config_load_defaults_and_clamps);
    RUN_TEST(test_config_all_sections);
    RUN_TEST(test_config_edge_cases);
    return result;
}
