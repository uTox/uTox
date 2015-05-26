#include "main.h"

/** Change source of main.c if windows or android
 *  else default to xlib
 **/
#ifdef __WIN32__
#include "windows/main.c"
#else
#ifdef __ANDROID__
#include "android/main.c"
#else
#ifdef __OBJC__
// #include "cocoa/main.m"
#else
#include "xlib/main.c"
#endif
#endif
#endif
