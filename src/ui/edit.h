#ifndef EDIT_H
#define EDIT_H

#include "../ui.h"
/*todo: replace windows functions, multiline edits, add missing edit functions (ex: double click to select word)*/

typedef struct scrollable SCROLLABLE;

typedef struct edit_change {
    bool     remove, padding;
    uint16_t start, length;
    char     data[];
} EDIT_CHANGE;

typedef struct edit EDIT;

struct edit {
    PANEL panel;

    bool multiline, mouseover, noborder, readonly, select_completely, vcentered, password;

    uint16_t mouseover_char, length, maxlength;
    uint16_t width, height;

    uint16_t      history_cur, history_length;
    EDIT_CHANGE **history;

    SCROLLABLE *scroll;
    char *      data;

    MAYBE_I18NAL_STRING empty_str;
    UI_ELEMENT_STYLE    style;

    void (*onenter)(EDIT *edit);
    void (*onchange)(EDIT *edit);
    void (*ontab)(EDIT *edit);
    void (*onshifttab)(EDIT *edit);
    void (*onlosefocus)(EDIT *edit);
};

void edit_draw(EDIT *edit, int x, int y, int width, int height);

bool edit_mmove(EDIT *edit, int x, int y, int width, int height, int mx, int my, int dx, int dy);
bool edit_mdown(EDIT *edit);
bool edit_dclick(EDIT *edit, bool triclick);
bool edit_mright(EDIT *edit);
bool edit_mwheel(EDIT *edit, int height, double d, bool smooth);
bool edit_mup(EDIT *edit);
bool edit_mleave(EDIT *edit);

void edit_do(EDIT *edit, uint16_t start, uint16_t length, bool remove);

void edit_press(void);

void edit_char(uint32_t ch, bool control, uint8_t flags);

int edit_selection(EDIT *edit, char *data, int len);
int edit_copy(char *data, int len);
void edit_paste(char *data, int len, bool select);

bool  edit_active(void);
EDIT *edit_get_active(void);

void edit_resetfocus(void);
void edit_setfocus(EDIT *edit);
void edit_setstr(EDIT *edit, char *str, uint16_t length);
void edit_setcursorpos(EDIT *edit, uint16_t pos);
uint16_t edit_getcursorpos(void);

// set outloc and outlen to the mark range.
// returns 1 if the mark range is valid for the current edit,
// else 0.
// a mark range is valid when *outlen != 0 and there is an active edit.
bool edit_getmark(uint16_t *outloc, uint16_t *outlen);
void edit_setmark(uint16_t loc, uint16_t len);

void edit_setselectedrange(uint16_t loc, uint16_t len);

extern EDIT edit_name, edit_status, edit_add_id, edit_add_msg, edit_msg, edit_msg_group, edit_search, edit_proxy_ip,
    edit_proxy_port, edit_profile_password, edit_nospam;

#endif
