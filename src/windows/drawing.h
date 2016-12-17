#ifndef WIN_DRAWING_H
#define WIN_DRAWING_H

#include <windows.h>

HDC target_DC; // Device Context of the window
HDC active_DC; // Device Context we draw onto
HBITMAP active_BM; // Bitmap we're currently drawing with

#endif
