#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600

#include <windows.h>
#include <windns.h>
#include <winreg.h>

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

#define debug(...) printf(__VA_ARGS__); fflush(stdout)

#define KEY(x) (x)

#ifdef __MINGW32__
#define fseeko fseeko64
#define ftello ftello64
#endif

// internal representation of an image
typedef struct utox_native_image {
    HBITMAP bitmap; // 32 bit bitmap containing
                    // red, green, blue and alpha

    _Bool has_alpha; // whether bitmap has an alpha channel

    // width and height in pixels of the bitmap
    uint32_t width, height;

    // width and height in pixels the image should be drawn to
    uint32_t scaled_width, scaled_height;

    // stretch mode used when stretching this image, either
    // COLORONCOLOR(ugly and fast), or HALFTONE(prettier and slower)
    int stretch_mode;

} UTOX_NATIVE_IMAGE;

#define UTOX_NATIVE_IMAGE_IS_VALID(x) (NULL != (x))
#define UTOX_NATIVE_IMAGE_HAS_ALPHA(x) (x->has_alpha)
