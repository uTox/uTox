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
        b = 1;
    }
    toxvideo_postmessage(VIDEO_SET, b, 0, handle);
}

static void dropdown_dpi_onselect(void *handle)
{
    ui_scale((size_t)handle);
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
        .name = (uint8_t*)"Large (150%)",
        .handle = (void*)(size_t)3
    },

    {
        .name = (uint8_t*)"Huge (200%)",
        .handle = (void*)(size_t)4
    }
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
    .dropcount = 4,
    .selected = 1,
    .over = 1,
    .drop = dpidrops
};
