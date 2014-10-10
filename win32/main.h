
#include <windows.h>
#include <windns.h>

#define KEY_BACK VK_BACK
#define KEY_RETURN VK_RETURN
#define KEY_LEFT VK_LEFT
#define KEY_RIGHT VK_RIGHT
#define KEY_TAB VK_TAB
#define KEY_DEL VK_DELETE
#define KEY_END VK_END
#define KEY_HOME VK_HOME
#define KEY_UP VK_UP
#define KEY_DOWN VK_DOWN
#define KEY_PAGEUP VK_PRIOR
#define KEY_PAGEDOWN VK_NEXT

#define debug(...) printf(__VA_ARGS__)

#define KEY(x) (x)

#ifdef __MINGW32__
#define fseeko fseeko64
#define ftello ftello64
#endif

typedef HBITMAP UTOX_NATIVE_IMAGE;
#define UTOX_NATIVE_IMAGE_IS_VALID(x) (NULL != (x))
