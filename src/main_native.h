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

void native_export_chatlog_init(uint32_t friend_number);


#endif
