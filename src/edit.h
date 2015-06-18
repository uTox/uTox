/*todo: replace windows functions, multiline edits, add missing edit functions (ex: double click to select word)*/

struct edit_change
{
    _Bool remove, padding;
    STRING_IDX start, length;
    char_t data[0];
};

struct edit {
    PANEL panel;

    _Bool multiline, mouseover, noborder, readonly, select_completely;
    STRING_IDX mouseover_char, length, maxlength;
    uint16_t width, height;

    uint16_t history_cur, history_length;
    EDIT_CHANGE **history;

    SCROLLABLE *scroll;
    char_t *data;

    MAYBE_I18NAL_STRING empty_str;
    UI_ELEMENT_STYLE style;

    void (*onenter)(EDIT *edit);
    void (*onchange)(EDIT *edit);
    void (*ontab)(EDIT *edit);
    void (*onshifttab)(EDIT *edit);
    void (*onlosefocus)(EDIT *edit);
};

void edit_draw(EDIT *edit, int x, int y, int width, int height);

_Bool edit_mmove(EDIT *edit, int x, int y, int width, int height, int mx, int my, int dx, int dy);
_Bool edit_mdown(EDIT *edit);
_Bool edit_dclick(EDIT *edit, _Bool triclick);
_Bool edit_mright(EDIT *edit);
_Bool edit_mwheel(EDIT *edit, int height, double d);
_Bool edit_mup(EDIT *edit);
_Bool edit_mleave(EDIT *edit);

void edit_do(EDIT *edit, STRING_IDX start, STRING_IDX length, _Bool remove);

void edit_press(void);

void edit_char(uint32_t ch, _Bool control, uint8_t flags);

int edit_selection(EDIT *edit, char_t *data, int len);
int edit_copy(char_t *data, int len);
void edit_paste(char_t *data, int len, _Bool select);

_Bool edit_active(void);
EDIT *edit_get_active(void);

void edit_resetfocus(void);
void edit_setfocus(EDIT *edit);
void edit_setstr(EDIT *edit, char_t *str, STRING_IDX length);
void edit_setcursorpos(EDIT *edit, STRING_IDX pos);
STRING_IDX edit_getcursorpos(void);

// set outloc and outlen to the mark range.
// returns 1 if the mark range is valid for the current edit,
// else 0.
// a mark range is valid when *outlen != 0 and there is an active edit.
_Bool edit_getmark(STRING_IDX *outloc, STRING_IDX *outlen);
void edit_setmark(STRING_IDX loc, STRING_IDX len);

void edit_setselectedrange(STRING_IDX loc, STRING_IDX len);
