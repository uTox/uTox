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
        .name = (uint8_t*)"English",
        .handle = (void*)(size_t)0
    },

    {
        .name = (uint8_t*)"French",
        .handle = (void*)(size_t)1
    },

    {
        .name = (uint8_t*)"Russian",
        .handle = (void*)(size_t)2
    },

    {
        .name = (uint8_t*)"Spanish",
        .handle = (void*)(size_t)3
    },

    {
        .name = (uint8_t*)"German",
        .handle = (void*)(size_t)4
    }
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
};
