#if defined(MAIN_H) && !defined(COCOA_MAIN_H)
#error "We should never include main from different platforms."
#endif

#ifndef COCOA_MAIN_H
#define COCOA_MAIN_H
#define MAIN_H

/* Don't put Objective-C declarations here so we don't have to compile
 * all of uTox as ObjC. Stuff that requires ObjC available goes in objc_main.h,
 * included below. */

#include <netinet/in.h>

#include <arpa/nameser_compat.h>

#include <resolv.h>

#include <errno.h>

typedef struct native_image NATIVE_IMAGE;
int NATIVE_IMAGE_IS_VALID(NATIVE_IMAGE *img);

#ifdef __OBJC__
#include "objc_main.h"
#endif

#endif
