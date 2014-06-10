
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <errno.h>

#define RGB(r, g, b) (((r) << 16) | ((g) << 8) | (b))

#define KEY_BACK XK_BackSpace
#define KEY_RETURN XK_Return

typedef struct
{
    int left, top, right, bottom;
}RECT;

typedef uint8_t char_t;

#define strcmp2(x, y) (memcmp(x, y, sizeof(y) - 1))
