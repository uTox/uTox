static void dropdown_audio_in_onselect(void *handle)
{
    toxav_postmessage(AV_SET_AUDIO_INPUT, 0, 0, handle);
}

static void dropdown_audio_out_onselect(void *handle)
{
    toxav_postmessage(AV_SET_AUDIO_OUTPUT, 0, 0, handle);
}

static void dropdown_video_onselect(void *handle)
{
    toxav_postmessage(AV_SET_VIDEO, 0, 0, handle);
}

DROPDOWN

dropdown_audio_in = {
    .panel = {
        .type = PANEL_DROPDOWN,
        .x = 5 * SCALE,
        .y = SCALE * 132,
        .height = SCALE * 12,
        .width = SCALE * 200
    },
    .onselect = dropdown_audio_in_onselect
},

dropdown_audio_out = {
    .panel = {
        .type = PANEL_DROPDOWN,
        .x = 5 * SCALE,
        .y = SCALE * 156,
        .height = SCALE * 12,
        .width = SCALE * 200
    },
    .onselect = dropdown_audio_out_onselect
},

dropdown_video = {
    .panel = {
        .type = PANEL_DROPDOWN,
        .x = 5 * SCALE,
        .y = SCALE * 180,
        .height = SCALE * 12,
        .width = SCALE * 200
    },
    .onselect = dropdown_video_onselect,
};
