#ifndef WIN_SCREENGRAB_H
#define WIN_SCREENGRAB_H

#include <windef.h>

void screen_grab_init(HINSTANCE app_instance);

void native_screen_grab_desktop(bool video);

#endif
