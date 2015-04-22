#ifndef COCOA_MAIN_H
#define COCOA_MAIN_H

/* Don't put Objective-C declarations here so we don't have to compile
 * all of uTox as ObjC. Stuff that requires ObjC available goes in objc_main.h,
 * included below. */

#include <netinet/in.h>

#include <arpa/nameser_compat.h>

#include <resolv.h>

#include <errno.h>

#include <Carbon/Carbon.h>

#define KEY_BACK     kVK_Delete
#define KEY_RETURN   kVK_Return
#define KEY_LEFT     kVK_LeftArrow
#define KEY_RIGHT    kVK_RightArrow
#define KEY_TAB      kVK_Tab
#define KEY_LEFT_TAB kVK_ISO_Left_Tab
#define KEY_DEL      kVK_ForwardDelete
#define KEY_END      kVK_End
#define KEY_HOME     kVK_Home
#define KEY_UP       kVK_UpArrow
#define KEY_DOWN     kVK_DownArrow
#define KEY_PAGEUP   kVK_PageUp
#define KEY_PAGEDOWN kVK_PageDown

#define KEY(k) (k)

#define RGB(r, g, b) (((r) << 16) | ((g) << 8) | (b))

void debug(const char *fmt, ...);
typedef struct utox_native_image UTOX_NATIVE_IMAGE;
int UTOX_NATIVE_IMAGE_IS_VALID(UTOX_NATIVE_IMAGE *img);

#ifdef __OBJC__
#include "objc_main.h"
#endif

#endif
