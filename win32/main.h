
#include <windows.h>
#include <windns.h>

#define KEY_BACK VK_BACK
#define KEY_RETURN VK_RETURN

typedef wchar_t char_t;



#define strcmp2(x, y) (memcmp(x, L##y, sizeof(y) * 2 - 1))
