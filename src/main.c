#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "main.h"

/** Change source of main.c if windows or android
 *  else default to xlib
 **/
#if defined __WIN32__
  //#include "windows/main.c"
#elif defined __ANDROID__
  #include "android/main.c"
#elif defined__OBJC__
  // #include "cocoa/main.m"
#else
  // #include "xlib/main.c"
#endif
