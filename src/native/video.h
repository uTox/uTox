#ifndef NATIVE_VIDEO_H
#define NATIVE_VIDEO_H

void video_frame(uint32_t id, uint8_t *img_data, uint16_t width, uint16_t height, bool resize);
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
