// Enums
/* uTox debug levels */
#ifndef NATIVE_MAIN_H
#define NATIVE_MAIN_H

#if defined __WIN32__
#include "windows/main.h"
#elif defined __ANDROID__
#include "android/main.h"
#elif defined __OBJC__
#include "cocoa/main.h"
#else
#include "xlib/main.h"
#endif

#ifndef FILE_OPTS
#define FILE_OPTS
typedef enum UTOX_FILE_OPTS {
    UTOX_FILE_OPTS_READ   = 1 << 0,
    UTOX_FILE_OPTS_WRITE  = 1 << 2,
    UTOX_FILE_OPTS_APPEND = 1 << 3,
    UTOX_FILE_OPTS_MKDIR  = 1 << 4,
} UTOX_FILE_OPTS;
#endif

uint16_t native_video_detect(void);
bool native_video_init(void *handle);
void native_video_close(void *handle);
int native_video_getframe(uint8_t *y, uint8_t *u, uint8_t *v, uint16_t width, uint16_t height);
bool  native_video_startread(void);
bool  native_video_endread(void);
FILE *native_get_file(char *name, size_t *size, UTOX_FILE_OPTS flag);

#endif
