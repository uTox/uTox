#if defined(MAIN_H) && !defined(ANDROID_MAIN_H)
#error "We should never include main from different platforms."
#endif

#ifndef ANDROID_MAIN_H
#define ANDROID_MAIN_H
#define MAIN_H

#include <android/log.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/system_properties.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <errno.h>

#define debug(...) ((void)__android_log_print(ANDROID_LOG_INFO, "utox", __VA_ARGS__))
#define debug_info(...) ((void)__android_log_print(ANDROID_LOG_INFO, "utox", __VA_ARGS__))
#define debug_notice(...) ((void)__android_log_print(ANDROID_LOG_INFO, "utox", __VA_ARGS__))
#define debug_error(...) ((void)__android_log_print(ANDROID_LOG_INFO, "utox", __VA_ARGS__))

#define RGB(r, g, b) ((r) | ((g) << 8) | ((b) << 16))

#define KEY_BACK 1
#define KEY_RETURN 2
#define KEY_LEFT 3
#define KEY_RIGHT 4
#define KEY_TAB 7
#define KEY_DEL 8
#define KEY_END 9
#define KEY_HOME 10
#define KEY_UP 5
#define KEY_DOWN 6
#define KEY_PAGEUP 11
#define KEY_PAGEDOWN 12

// Early include to obtain GLuint.
#include <GLES2/gl2.h>
typedef struct android_native_image { GLuint img } NATIVE_IMAGE;

#define NATIVE_IMAGE_IS_VALID(x) (0 != (x))

#define ANDROID_INTERNAL_SAVE "/data/data/tox.client.utox/files/"

#endif
