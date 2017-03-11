#if defined(MAIN_H) && !defined(ANDROID_MAIN_H)
#error "We should never include main from different platforms."
#endif

#ifndef ANDROID_MAIN_H
#define ANDROID_MAIN_H
#define MAIN_H

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/system_properties.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

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
typedef struct native_image { GLuint img; } NATIVE_IMAGE;

#define NATIVE_IMAGE_IS_VALID(x) (0 != (x))

#define ANDROID_INTERNAL_SAVE "/data/data/tox.client.utox/files/"

#endif
