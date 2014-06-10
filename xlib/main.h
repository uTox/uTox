
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <errno.h>

#define RGB(r, g, b) (((r) << 16) | ((g) << 8) | (b))

#define KEY_BACK XK_BackSpace
#define KEY_RETURN XK_Return

#define USENATIVECONTROLS

typedef struct
{
    int left, top, right, bottom;
}RECT;
