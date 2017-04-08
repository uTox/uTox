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

// Early include to obtain GLuint.
#include <GLES2/gl2.h>
typedef struct android_native_image { GLuint img; } NATIVE_IMAGE;

#define ANDROID_INTERNAL_SAVE "/data/data/tox.client.utox/files/"

#endif
