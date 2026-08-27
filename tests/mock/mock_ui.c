#include "mock_ui.h"

#include "../../src/theme.h"
#include "../../src/ui/draw.h"

#include <stdio.h>
#include <string.h>

double ui_scale = 10.0;
int font_small_lineheight = 12;
int font_msg_lineheight   = 16;

struct utox_mouse mouse;
uint8_t cursor;
bool mdown;

int mock_draw_fill_count;
int mock_draw_frame_count;
int mock_draw_alpha_count;
int mock_draw_text_count;
int mock_last_font;
uint32_t mock_last_color;

int mock_thread_count;
UTOX_MSG mock_last_utox_msg;

uint32_t COLOR_BKGRND_MAIN, COLOR_BKGRND_ALT, COLOR_BKGRND_AUX, COLOR_BKGRND_MENU;
uint32_t COLOR_BKGRND_MENU_HOVER, COLOR_BKGRND_MENU_ACTIVE, COLOR_BKGRND_LIST, COLOR_BKGRND_LIST_HOVER;
uint32_t COLOR_MAIN_TEXT, COLOR_MAIN_TEXT_CHAT, COLOR_MAIN_TEXT_SUBTEXT, COLOR_MAIN_TEXT_ACTION;
uint32_t COLOR_MAIN_TEXT_QUOTE, COLOR_MAIN_TEXT_RED, COLOR_MAIN_TEXT_URL, COLOR_MAIN_TEXT_HINT;
uint32_t COLOR_MSG_USER, COLOR_MSG_USER_PEND, COLOR_MSG_USER_ERROR, COLOR_MSG_CONTACT;
uint32_t COLOR_MENU_TEXT, COLOR_MENU_TEXT_SUBTEXT, COLOR_MENU_TEXT_ACTIVE;
uint32_t COLOR_LIST_TEXT, COLOR_LIST_TEXT_SUBTEXT;
uint32_t COLOR_AUX_EDGE_NORMAL, COLOR_AUX_EDGE_HOVER, COLOR_AUX_EDGE_ACTIVE, COLOR_AUX_TEXT;
uint32_t COLOR_AUX_ACTIVEOPTION_BKGRND, COLOR_AUX_ACTIVEOPTION_TEXT;
uint32_t COLOR_GROUP_SELF, COLOR_GROUP_PEER, COLOR_GROUP_AUDIO, COLOR_GROUP_MUTED;
uint32_t COLOR_SELECTION_BACKGROUND, COLOR_SELECTION_TEXT;
uint32_t COLOR_EDGE_NORMAL, COLOR_EDGE_ACTIVE, COLOR_EDGE_HOVER;
uint32_t COLOR_ACTIVEOPTION_BKGRND, COLOR_ACTIVEOPTION_TEXT;
uint32_t COLOR_STATUS_ONLINE, COLOR_STATUS_AWAY, COLOR_STATUS_BUSY;
uint32_t COLOR_BTN_SUCCESS_BKGRND, COLOR_BTN_SUCCESS_TEXT, COLOR_BTN_SUCCESS_BKGRND_HOVER, COLOR_BTN_SUCCESS_TEXT_HOVER;
uint32_t COLOR_BTN_WARNING_BKGRND, COLOR_BTN_WARNING_TEXT, COLOR_BTN_WARNING_BKGRND_HOVER, COLOR_BTN_WARNING_TEXT_HOVER;
uint32_t COLOR_BTN_DANGER_BACKGROUND, COLOR_BTN_DANGER_TEXT, COLOR_BTN_DANGER_BKGRND_HOVER, COLOR_BTN_DANGER_TEXT_HOVER;
uint32_t COLOR_BTN_DISABLED_BKGRND, COLOR_BTN_DISABLED_TEXT, COLOR_BTN_DISABLED_BKGRND_HOVER, COLOR_BTN_DISABLED_TRANSFER;
uint32_t COLOR_BTN_INPROGRESS_BKGRND, COLOR_BTN_INPROGRESS_TEXT, COLOR_BTN_DISABLED_FORGRND, COLOR_BTN_INPROGRESS_FORGRND;
uint32_t status_color[4];

void mock_ui_reset(void) {
    mock_draw_fill_count  = 0;
    mock_draw_frame_count = 0;
    mock_draw_alpha_count = 0;
    mock_draw_text_count  = 0;
    mock_last_font        = 0;
    mock_last_color       = 0;
    mock_thread_count     = 0;
    mock_last_utox_msg    = 0;
    cursor                = CURSOR_NONE;
    mdown                 = false;
    mouse.x               = 0;
    mouse.y               = 0;
    ui_scale              = 10.0;
}

void maybe_i18nal_string_set_plain(MAYBE_I18NAL_STRING *mis, char *str, uint16_t length) {
    mis->i18nal       = UI_STRING_ID_INVALID;
    mis->plain.length = length;
    mis->plain.str    = str;
}

void maybe_i18nal_string_set_i18nal(MAYBE_I18NAL_STRING *mis, UTOX_I18N_STR string_id) {
    mis->plain.str    = NULL;
    mis->plain.length = 0;
    mis->i18nal       = string_id;
}

STRING *maybe_i18nal_string_get(MAYBE_I18NAL_STRING *mis) {
    if (mis->plain.str) {
        return &mis->plain;
    }

    return SPTRFORLANG(settings.language, mis->i18nal);
}

bool maybe_i18nal_string_is_valid(MAYBE_I18NAL_STRING *mis) {
    return (mis->plain.str || ((UI_STRING_ID_INVALID != mis->i18nal) && (mis->i18nal < NUM_STRS)));
}

STRING *ui_gettext(UTOX_LANG lang, UTOX_I18N_STR string_id) {
    (void)lang;
    static STRING s;
    static char buf[32];
    snprintf(buf, sizeof buf, "i18n-%u", (unsigned)string_id);
    s.str    = buf;
    s.length = (uint16_t)strlen(buf);
    return &s;
}

/* Do not run the callback — tooltip_thread loops until kill_thread. */
void thread(void func(void *), void *args) {
    (void)func;
    (void)args;
    mock_thread_count++;
}

void postmessage_utox(UTOX_MSG msg, uint16_t param1, uint16_t param2, void *data) {
    (void)param1;
    (void)param2;
    (void)data;
    mock_last_utox_msg = msg;
}

void drawtext(int x, int y, const char *str, uint16_t length) {
    (void)x;
    (void)y;
    (void)str;
    (void)length;
    mock_draw_text_count++;
}

int drawtext_getwidth(int x, int y, const char *str, uint16_t length) {
    (void)x;
    (void)y;
    return textwidth(str, length);
}

void drawtextwidth(int x, int width, int y, const char *str, uint16_t length) {
    (void)x;
    (void)width;
    (void)y;
    (void)str;
    (void)length;
    mock_draw_text_count++;
}

void drawtextwidth_right(int x, int width, int y, const char *str, uint16_t length) {
    (void)x;
    (void)width;
    (void)y;
    (void)str;
    (void)length;
}

void drawtextrange(int x, int x2, int y, const char *str, uint16_t length) {
    (void)x;
    (void)x2;
    (void)y;
    (void)str;
    (void)length;
}

void drawtextrangecut(int x, int x2, int y, const char *str, uint16_t length) {
    (void)x;
    (void)x2;
    (void)y;
    (void)str;
    (void)length;
}

int textwidth(const char *str, uint16_t length) {
    (void)str;
    return length;
}

int textfit(const char *str, uint16_t length, int width) {
    (void)str;
    if (width <= 0) {
        return 0;
    }
    return length < (uint16_t)width ? length : width;
}

int textfit_near(const char *str, uint16_t length, int width) {
    return textfit(str, length, width);
}

void drawrect(int x, int y, int right, int bottom, uint32_t color) {
    (void)x;
    (void)y;
    (void)right;
    (void)bottom;
    (void)color;
}

void draw_rect_frame(int x, int y, int width, int height, uint32_t color) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
    mock_draw_frame_count++;
}

void draw_rect_fill(int x, int y, int width, int height, uint32_t color) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
    mock_draw_fill_count++;
}

void drawhline(int x, int y, int x2, uint32_t color) {
    (void)x;
    (void)y;
    (void)x2;
    (void)color;
}

void drawvline(int x, int y, int y2, uint32_t color) {
    (void)x;
    (void)y;
    (void)y2;
    (void)color;
}

void setfont(int id) {
    mock_last_font = id;
}

uint32_t setcolor(uint32_t color) {
    uint32_t prev    = mock_last_color;
    mock_last_color = color;
    return prev;
}

void pushclip(int x, int y, int width, int height) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void popclip(void) {}

void enddraw(int x, int y, int width, int height) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void drawalpha(int bm, int x, int y, int width, int height, uint32_t color) {
    (void)bm;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
    mock_draw_alpha_count++;
}

void loadalpha(int bm, void *data, int width, int height) {
    (void)bm;
    (void)data;
    (void)width;
    (void)height;
}
