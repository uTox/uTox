

#include <X11/Xutil.h>

#include <netinet/in.h>

#ifdef __APPLE__
#include <arpa/nameser_compat.h>
#else
#include <arpa/nameser.h>
#endif

#include <resolv.h>

#include <errno.h>

#define debug(...) printf(__VA_ARGS__)

#define RGB(r, g, b) (((r) << 16) | ((g) << 8) | (b))

#define KEY_BACK XK_BackSpace
#define KEY_RETURN XK_Return
#define KEY_LEFT XK_Left
#define KEY_RIGHT XK_Right
#define KEY_TAB XK_Tab
#define KEY_DEL XK_Delete
#define KEY_END XK_End
#define KEY_HOME XK_Home
#define KEY_UP XK_Up
#define KEY_DOWN XK_Down
#define KEY_PAGEUP XK_Page_Up
#define KEY_PAGEDOWN XK_Page_Down

#define KEY(x) (x + 'a' - 'A')
