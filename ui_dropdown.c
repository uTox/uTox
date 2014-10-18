#include "main.h"

static void dropdown_audio_in_onselect(uint16_t i, const DROPDOWN* dm)
{
    DROP_ELEMENT *e = &((DROP_ELEMENT*) dm->userdata)[i];
    void *handle = e->handle;
    toxaudio_postmessage(AUDIO_SET_INPUT, 0, 0, handle);
}

static void dropdown_audio_out_onselect(uint16_t i, const DROPDOWN* dm)
{
    DROP_ELEMENT *e = &((DROP_ELEMENT*) dm->userdata)[i];
    void *handle = e->handle;
    toxaudio_postmessage(AUDIO_SET_OUTPUT, 0, 0, handle);
}

static void dropdown_video_onselect(uint16_t i, const DROPDOWN* dm)
{
    DROP_ELEMENT *e = &((DROP_ELEMENT*) dm->userdata)[i];
    void *handle = e->handle;
    uint16_t b = 0;
    if(!handle && video_preview) {
        video_end(0);
        video_preview = 0;
        b = 1; //tell video thread to stop preview too
    } else if((size_t)handle == 1) {
        desktopgrab(1);
        return;
    }

    toxvideo_postmessage(VIDEO_SET, b, 0, handle);
}

static void dropdown_dpi_onselect(uint16_t i, const DROPDOWN* UNUSED(dm))
{
    ui_scale(i + 1);
}

static void dropdown_language_onselect(uint16_t i, const DROPDOWN* UNUSED(dm))
{
    LANG = (UI_LANG_ID)i;
}
static STRING* dropdown_language_ondisplay(uint16_t i, const DROPDOWN* UNUSED(dm))
{
    UI_LANG_ID l = (UI_LANG_ID)i;
    return SPTRFORLANG(l, STR_LANG_NATIVE_NAME);
}

static void dropdown_filter_onselect(uint16_t i, const DROPDOWN* UNUSED(dm))
{
    FILTER = !!i;
}

static void dropdown_proxy_onselect(uint16_t i, const DROPDOWN* UNUSED(dm))
{
    if((i != 0) != (options.proxy_enabled) || i) {
        options.proxy_enabled = (i != 0);
        if(i == 2 && !options.udp_disabled) {
            options.udp_disabled = 1;
            dropdown_udp.selected = dropdown_udp.over = 1;
        }
        memcpy(options.proxy_address, edit_proxy_ip.data, edit_proxy_ip.length);
        options.proxy_address[edit_proxy_ip.length] = 0;

        edit_proxy_port.data[edit_proxy_port.length] = 0;
        options.proxy_port = strtol((char*)edit_proxy_port.data, NULL, 0);

        tox_settingschanged();
    }
}

static void dropdown_ipv6_onselect(uint16_t i, const DROPDOWN* UNUSED(dm))
{
    if(!i != options.ipv6enabled) {
        options.ipv6enabled = !i;
        tox_settingschanged();
    }
}

static void dropdown_udp_onselect(uint16_t i, const DROPDOWN* UNUSED(dm))
{
    if(i != options.udp_disabled) {
        options.udp_disabled = i;
        if(!i && dropdown_proxy.selected == 2) {
            dropdown_proxy.selected = dropdown_proxy.over = 1;
        }
        tox_settingschanged();
    }
}

static void dropdown_logging_onselect(uint16_t i, const DROPDOWN* UNUSED(dm))
{
    logging_enabled = !!i;
}

static void dropdown_notifications_onselect(uint16_t i, const DROPDOWN* UNUSED(dm))
{
    audible_notifications_enabled = !i;
}




static UI_STRING_ID dpidrops[] = {
    STR_DPI_TINY,
    STR_DPI_NORMAL,
    STR_DPI_BIG,
    STR_DPI_LARGE,
    STR_DPI_HUGE,
};

static UI_STRING_ID filterdrops[] = {
    STR_CONTACTS_FILTER_ALL,
    STR_CONTACTS_FILTER_ONLINE,
};

static UI_STRING_ID proxydrops[] = {
    STR_PROXY_DISABLED,
    STR_PROXY_FALLBACK,
    STR_PROXY_ALWAYS_USE,
};

static UI_STRING_ID yesnodrops[] = {STR_YES, STR_NO};

static UI_STRING_ID noyesdrops[] = {STR_NO, STR_YES};

DROPDOWN

dropdown_audio_in = {
    .ondisplay = list_dropdown_ondisplay,
    .onselect = dropdown_audio_in_onselect
},

dropdown_audio_out = {
    .ondisplay = list_dropdown_ondisplay,
    .onselect = dropdown_audio_out_onselect
},

dropdown_video = {
    .ondisplay = list_dropdown_ondisplay,
    .onselect = dropdown_video_onselect,
},

dropdown_dpi = {
    .ondisplay = simple_dropdown_ondisplay,
    .onselect = dropdown_dpi_onselect,
    .dropcount = countof(dpidrops),
    .userdata = dpidrops
},

dropdown_language = {
    .ondisplay = dropdown_language_ondisplay,
    .onselect = dropdown_language_onselect,
    .dropcount = LANGS_MAX+1,
},

dropdown_filter = {
    .ondisplay = simple_dropdown_ondisplay,
    .onselect = dropdown_filter_onselect,
    .dropcount = countof(filterdrops),
    .userdata = filterdrops
},

dropdown_proxy = {
    .ondisplay = simple_dropdown_ondisplay,
    .onselect = dropdown_proxy_onselect,
    .dropcount = countof(proxydrops),
    .userdata = proxydrops
},

dropdown_ipv6 = {
    .ondisplay = simple_dropdown_ondisplay,
    .onselect = dropdown_ipv6_onselect,
    .dropcount = countof(yesnodrops),
    .userdata = yesnodrops
},

dropdown_udp = {
    .ondisplay = simple_dropdown_ondisplay,
    .onselect = dropdown_udp_onselect,
    .dropcount = countof(yesnodrops),
    .userdata = yesnodrops
},

dropdown_logging = {
    .ondisplay = simple_dropdown_ondisplay,
    .onselect = dropdown_logging_onselect,
    .dropcount = countof(noyesdrops),
    .userdata = noyesdrops
},

dropdown_audible_notification = {
    .ondisplay = simple_dropdown_ondisplay,
    .onselect = dropdown_notifications_onselect,
    .dropcount = countof(yesnodrops),
    .userdata = yesnodrops
};
