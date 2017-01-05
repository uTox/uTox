#include "main.h"

#include "../tox.h"
#include "../flist.h"

#include <windowsx.h>

static HWND      grab_window;
static HINSTANCE grab_instance;
static bool desktopgrab_video = false;

// creates an UTOX_NATIVE image based on given arguments
// image should be freed with image_free
static NATIVE_IMAGE *create_utox_image(HBITMAP bmp, bool has_alpha, uint32_t width, uint32_t height) {
    NATIVE_IMAGE *image = malloc(sizeof(NATIVE_IMAGE));
    if (image == NULL) {
        debug("create_utox_image:\t Could not allocate memory for image.\n");
        return NULL;
    }
    image->bitmap        = bmp;
    image->has_alpha     = has_alpha;
    image->width         = width;
    image->height        = height;
    image->scaled_width  = width;
    image->scaled_height = height;
    image->stretch_mode  = COLORONCOLOR;

    return image;
}

static void sendbitmap(HDC mem, HBITMAP hbm, int width, int height) {
    if (width == 0 || height == 0)
        return;

    BITMAPINFO info = {
        .bmiHeader = {
            .biSize        = sizeof(BITMAPINFOHEADER),
            .biWidth       = width,
            .biHeight      = -(int)height,
            .biPlanes      = 1,
            .biBitCount    = 24,
            .biCompression = BI_RGB,
        }
    };

    void *bits = malloc((width + 3) * height * 3);

    GetDIBits(mem, hbm, 0, height, bits, &info, DIB_RGB_COLORS);

    uint8_t pbytes = width & 3, *p = bits, *pp = bits, *end = p + width * height * 3;
    // uint32_t offset = 0;
    while (p != end) {
        int i;
        for (i = 0; i != width; i++) {
            uint8_t b    = pp[i * 3];
            p[i * 3]     = pp[i * 3 + 2];
            p[i * 3 + 1] = pp[i * 3 + 1];
            p[i * 3 + 2] = b;
        }
        p += width * 3;
        pp += width * 3 + pbytes;
    }

    int size = 0;
    uint8_t *out = stbi_write_png_to_mem(bits, 0, width, height, 3, &size);

    free(bits);

    NATIVE_IMAGE *image = create_utox_image(hbm, 0, width, height);
    friend_sendimage(flist_get_selected()->data, image, width, height, (UTOX_IMAGE)out, size);
}

static LRESULT CALLBACK screen_grab_sys(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    POINT p = {.x = GET_X_LPARAM(lParam), .y = GET_Y_LPARAM(lParam) };

    ClientToScreen(window, &p);

    static bool mdown = false;
    switch (msg) {
        case WM_MOUSEMOVE: {
            if (!mdown) {
                break;
            }

            HDC dc = GetDC(window);
            BitBlt(dc, video_grab_x, video_grab_y, video_grab_w - video_grab_x, video_grab_h - video_grab_y, dc,
                       video_grab_x, video_grab_y, BLACKNESS);
            video_grab_w = p.x;
            video_grab_h = p.y;
            BitBlt(dc, video_grab_x, video_grab_y, video_grab_w - video_grab_x, video_grab_h - video_grab_y, dc,
                       video_grab_x, video_grab_y, WHITENESS);
            ReleaseDC(window, dc);
            return false;
        }

        case WM_LBUTTONDOWN: {
            mdown = true;
            video_grab_x = video_grab_w = p.x;
            video_grab_y = video_grab_h = p.y;
            SetCapture(window);
            return false;
        }

        case WM_LBUTTONUP: {
            mdown = false;
            ReleaseCapture();

            if (video_grab_x < video_grab_w) {
                video_grab_w -= video_grab_x;
            } else {
                const int w  = video_grab_x - video_grab_w;
                video_grab_x = video_grab_w;
                video_grab_w = w;
            }

            if (video_grab_y < video_grab_h) {
                video_grab_h -= video_grab_y;
            } else {
                const int w  = video_grab_y - video_grab_h;
                video_grab_y = video_grab_h;
                video_grab_h = w;
            }

            if (desktopgrab_video) {
                DestroyWindow(window);
                postmessage_utoxav(UTOXAV_SET_VIDEO_IN, 1, 0, NULL);
            } else {
                FRIEND *f = flist_get_selected()->data;
                if (flist_get_selected()->item == ITEM_FRIEND && f->online) {
                    DestroyWindow(window);
                    HWND dwnd = GetDesktopWindow();
                    HDC  ddc  = GetDC(dwnd);
                    HDC  mem  = CreateCompatibleDC(ddc);

                    HBITMAP capture = CreateCompatibleBitmap(ddc, video_grab_w, video_grab_h);
                    SelectObject(mem, capture);

                    BitBlt(mem, 0, 0, video_grab_w, video_grab_h, ddc, video_grab_x, video_grab_y, SRCCOPY | CAPTUREBLT);
                    sendbitmap(mem, capture, video_grab_w, video_grab_h);

                    ReleaseDC(dwnd, ddc);
                    DeleteDC(mem);
                }
            }
            return false;
        }
    }

    return DefWindowProcW(window, msg, wParam, lParam);
}

void screen_grab_init(HINSTANCE app_instance) {
    HICON black_icon  = LoadIcon(app_instance, MAKEINTRESOURCE(101));
    wchar_t screen_grab_class[] = L"uToxgrab";

    WNDCLASSW grab_window_class = {
        .hInstance     = app_instance,
        .lpfnWndProc   = screen_grab_sys,
        .lpszClassName = screen_grab_class,
        .hIcon         = black_icon,
        .hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH),
    };
    RegisterClassW(&grab_window_class);

    grab_instance = app_instance;
}

void native_screen_grab_desktop(bool video) {
    int x, y, w, h;

    x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    grab_window = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_LAYERED, L"uToxgrab", L"Tox", WS_POPUP,
                                 x, y, w, h,
                                 NULL, NULL, grab_instance, NULL);
    if (!grab_window) {
        debug_error("CreateWindowExW() failed\n");
        return;
    }

    SetLayeredWindowAttributes(grab_window, 0xFFFFFF, 128, LWA_ALPHA | LWA_COLORKEY);
    // UpdateLayeredWindow(main_window.window, NULL, NULL, NULL, NULL, NULL, 0xFFFFFF, ULW_ALPHA | ULW_COLORKEY);

    ShowWindow(grab_window, SW_SHOW);
    SetForegroundWindow(grab_window);

    desktopgrab_video = video;
}
