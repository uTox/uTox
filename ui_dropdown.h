static void dropdown_audio_in_onselect(void *handle)
{
    toxaudio_postmessage(AUDIO_SET_INPUT, 0, 0, handle);
}

static void dropdown_audio_out_onselect(void *handle)
{
    toxaudio_postmessage(AUDIO_SET_OUTPUT, 0, 0, handle);
}

static void dropdown_video_onselect(void *handle)
{
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

static void dropdown_dpi_onselect(void *handle)
{
    ui_scale((size_t)handle);
}

static void dropdown_language_onselect(void *handle)
{
    LANG = (size_t)handle;
}

static void dropdown_filter_onselect(void *handle)
{
    FILTER = (size_t)handle;
}

static void dropdown_proxy_onselect(void *handle)
{
    uint8_t i = (size_t)handle;
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

static void dropdown_ipv6_onselect(void *handle)
{
    uint8_t i = (size_t)handle;
    if(!i != options.ipv6enabled) {
        options.ipv6enabled = !i;
        tox_settingschanged();
    }
}

static void dropdown_udp_onselect(void *handle)
{
    uint8_t i = (size_t)handle;
    if(i != options.udp_disabled) {
        options.udp_disabled = i;
        if(!i && dropdown_proxy.selected == 2) {
            dropdown_proxy.selected = dropdown_proxy.over = 1;
        }
        tox_settingschanged();
    }
}

static void dropdown_logging_onselect(void *handle)
{
    logging_enabled = (handle != 0);
}

static DROP_ELEMENT dpidrops[] = {
    {
        .name = (uint8_t*)"Tiny (50%)",
        .handle = (void*)(size_t)1
    },

    {
        .name = (uint8_t*)"Normal (100%)",
        .handle = (void*)(size_t)2
    },

    {
        .name = (uint8_t*)"Big (150%)",
        .handle = (void*)(size_t)3
    },

    {
        .name = (uint8_t*)"Large (200%)",
        .handle = (void*)(size_t)4
    },

    {
        .name = (uint8_t*)"Huge (250%)",
        .handle = (void*)(size_t)5
    }
};

static DROP_ELEMENT langdrops[] = {
    {
        .name = (uint8_t*)"Deutsch",
        .handle = (void*)(size_t)0
    },

    {
        .name = (uint8_t*)"English",
        .handle = (void*)(size_t)1
    },

    {
        .name = (uint8_t*)"Español",
        .handle = (void*)(size_t)2
    },

    {
        .name = (uint8_t*)"Français",
        .handle = (void*)(size_t)3
    },

    {
        .name = (uint8_t*)"Italiano",
        .handle = (void*)(size_t)4
    },

    {
        .name = (uint8_t*)"日本語",
        .handle = (void*)(size_t)5
    },

    {
        .name = (uint8_t*)"Latviešu",
        .handle = (void*)(size_t)6
    },

    {
        .name = (uint8_t*)"Nederlands",
        .handle = (void*)(size_t)7
    },

    {
        .name = (uint8_t*)"Polski",
        .handle = (void*)(size_t)8
    },

    {
        .name = (uint8_t*)"Русский",
        .handle = (void*)(size_t)9
    },

    {
        .name = (uint8_t*)"Українська",
        .handle = (void*)(size_t)10
    },

    {
        .name = (uint8_t*)"简体中文",
        .handle = (void*)(size_t)11
    },
};

static DROP_ELEMENT filterdrops[] = {
    {
        .name = (uint8_t*)"All",
        .handle = (void*)(size_t)0
    },

    {
        .name = (uint8_t*)"Online",
        .handle = (void*)(size_t)1
    },
};


static DROP_ELEMENT proxydrops[] = {
    {
        .name = (uint8_t*)"Disabled",
        .handle = (void*)(size_t)0
    },

    {
        .name = (uint8_t*)"Fallback",
        .handle = (void*)(size_t)1
    },

    {
        .name = (uint8_t*)"Always use",
        .handle = (void*)(size_t)2
    },
};

static DROP_ELEMENT yesnodrops[] = {
    {
        .name = (uint8_t*)"Yes",
        .handle = (void*)(size_t)0
    },

    {
        .name = (uint8_t*)"No",
        .handle = (void*)(size_t)1
    },
};

static DROP_ELEMENT noyesdrops[] = {
    {
        .name = (uint8_t*)"No",
        .handle = (void*)(size_t)0
    },

    {
        .name = (uint8_t*)"Yes",
        .handle = (void*)(size_t)1
    },
};

DROPDOWN

dropdown_audio_in = {
    .onselect = dropdown_audio_in_onselect
},

dropdown_audio_out = {
    .onselect = dropdown_audio_out_onselect
},

dropdown_video = {
    .onselect = dropdown_video_onselect,
},

dropdown_dpi = {
    .onselect = dropdown_dpi_onselect,
    .dropcount = countof(dpidrops),
    .drop = dpidrops
},

dropdown_language = {
    .onselect = dropdown_language_onselect,
    .dropcount = countof(langdrops),
    .drop = langdrops
},

dropdown_filter = {
    .onselect = dropdown_filter_onselect,
    .dropcount = countof(filterdrops),
    .drop = filterdrops
},

dropdown_proxy = {
    .onselect = dropdown_proxy_onselect,
    .dropcount = countof(proxydrops),
    .drop = proxydrops
},

dropdown_ipv6 = {
    .onselect = dropdown_ipv6_onselect,
    .dropcount = countof(yesnodrops),
    .drop = yesnodrops
},

dropdown_udp = {
    .onselect = dropdown_udp_onselect,
    .dropcount = countof(yesnodrops),
    .drop = yesnodrops
},

dropdown_logging = {
    .onselect = dropdown_logging_onselect,
    .dropcount = countof(noyesdrops),
    .drop = noyesdrops
};
