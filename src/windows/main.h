#if defined(MAIN_H) && !defined(WINDOWS_MAIN_H)
#error "We should never include main from different platforms."
#endif

#ifndef WINDOWS_MAIN_H
#define WINDOWS_MAIN_H
#define MAIN_H

/** Select the true main.c for legacy XP support.
 *  else default to xlib
 **/
#ifndef __WIN_LEGACY
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifndef WINVER
#define WINVER 0x410
#endif

#undef CLEARTYPE_QUALITY
#define CLEARTYPE_QUALITY 5

#define STRSAFE_NO_DEPRECATE
#include <windns.h>
#include <windows.h>
#include <winreg.h>

#ifdef __MINGW32__
#define fseeko fseeko64
#define ftello ftello64
#endif

#define STRSAFE_NO_DEPRECATE

#ifdef __CRT__NO_INLINE
#undef __CRT__NO_INLINE
#define DID_UNDEFINE__CRT__NO_INLINE
#include <dshow.h>
#ifdef DID_UNDEFINE__CRT__NO_INLINE
#define __CRT__NO_INLINE
#endif
#endif

#include <initguid.h>

#include <amvideo.h>
#include <control.h>
#include <strmif.h>
#include <uuids.h>
#include <vfwmsgs.h>

#include <qedit.h>
extern const CLSID CLSID_SampleGrabber;
extern const CLSID CLSID_NullRenderer;

#include <audioclient.h>
#include <mmdeviceapi.h>
#include <process.h>

#include <knownfolders.h>
#include <shlobj.h>

#include <error.h>
#include <io.h>

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

#define debug(...) (settings.verbose >= VERB_TEENAGE_GIRL) ? (printf(__VA_ARGS__) & fflush(stdout)) : (0)
#define debug_info(...) (settings.verbose >= VERB_NEW_ADHD_MEDS) ? (printf(__VA_ARGS__) & fflush(stdout)) : (0)
#define debug_notice(...) (settings.verbose >= VERB_CONCERNED_PARENT) ? (printf(__VA_ARGS__) & fflush(stdout)) : (0)
#define debug_error(...) (settings.verbose >= VERB_JANICE_ACCOUNTING) ? (printf(__VA_ARGS__) & fflush(stdout)) : (0)

#define WM_NOTIFYICON (WM_APP + 0)
#define WM_TOX (WM_APP + 1)

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
/* Included in dnd.c */
void dnd_init(HWND window);

enum {
    MENU_TEXTINPUT = 101,
    MENU_MESSAGES  = 102,
};

// HBITMAP bitmap[32];
void *  bitmap[BM_ENDMARKER + 1];
HFONT   font[32];
HCURSOR cursors[8];
HICON   my_icon, unread_messages_icon;

HWND      hwnd, capturewnd;
HINSTANCE hinstance;
HDC       main_hdc, hdc, hdcMem;
HBRUSH    hdc_brush;
HBITMAP   hdc_bm;
HWND      video_hwnd[MAX_NUM_FRIENDS];

// internal representation of an image
typedef struct utox_native_image {
    HBITMAP bitmap; // 32 bit bitmap containing
                    // red, green, blue and alpha

    bool has_alpha; // whether bitmap has an alpha channel

    // width and height in pixels of the bitmap
    uint32_t width, height;

    // width and height in pixels the image should be drawn to
    uint32_t scaled_width, scaled_height;

    // stretch mode used when stretching this image, either
    // COLORONCOLOR(ugly and fast), or HALFTONE(prettier and slower)
    int stretch_mode;

} NATIVE_IMAGE;

#define NATIVE_IMAGE_IS_VALID(x) (NULL != (x))
#define NATIVE_IMAGE_HAS_ALPHA(x) (x->has_alpha)

// static char save_path[280];
char portable_mode_save_path[MAX_PATH];

// WM_COMMAND
enum {
    TRAY_SHOWHIDE,
    TRAY_EXIT,
    TRAY_STATUS_AVAILABLE,
    TRAY_STATUS_AWAY,
    TRAY_STATUS_BUSY,
};

// TODO move these into os_video.c
int  video_grab_x, video_grab_y, video_grab_w, video_grab_h;
bool grabbing;

int native_to_utf8str(wchar *str_in, char *str_out, uint32_t max_size);

#endif
