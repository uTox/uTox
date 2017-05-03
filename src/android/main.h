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

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

// Early include to obtain GLuint.
#include <GLES2/gl2.h>
typedef struct native_image { GLuint img; } NATIVE_IMAGE;

#define ANDROID_INTERNAL_SAVE "/data/data/tox.client.utox/files/"

#endif
