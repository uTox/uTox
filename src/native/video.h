#ifndef NATIVE_VIDEO_H
#define NATIVE_VIDEO_H

#include <stdint.h>

void video_frame(uint32_t id, uint8_t *img_data, uint16_t width, uint16_t height, bool resize);

/**
 * Opens the OS window for the incoming video frames
 *
 * @param id          The id number of the friend
 *                    Use UINT16_MAX for the preview window
 *                    currently any value > UINT16_MAX will be treaded as the
 *                    preview window
 *                    TODO: move this window handle to the friend struct?
 *                    TODO: fix static alloc of the window handles
 * @param name        Name for the title of the window
 * @param name_length length of @name in bytes
 * @param width       starting size of the video frame
 * @param height      starting size of the video frame
 */
void video_begin(uint32_t id, char *name, uint16_t name_length, uint16_t width, uint16_t height);


void video_end(uint32_t id);

uint16_t native_video_detect(void);
bool native_video_init(void *handle);
void native_video_close(void *handle);
int native_video_getframe(uint8_t *y, uint8_t *u, uint8_t *v, uint16_t width, uint16_t height);
bool native_video_startread(void);
bool native_video_endread(void);

// OS X only.
void desktopgrab(bool video);

#endif
