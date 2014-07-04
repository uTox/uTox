/*todo: replace windows functions, multiline edits, add missing edit functions (ex: double click to select word)*/

struct edit_change
{
    _Bool remove;
    uint16_t start, length;
    EDIT_CHANGE *last, *next;
    uint8_t data[0];
};

struct edit {
    PANEL panel;

    _Bool multiline, mouseover, noborder, readonly;
    uint16_t mouseover_char, length, maxlength, width;
    EDIT_CHANGE *current, *next, *last;
    SCROLLABLE *scroll;
    char_t *data;
    void (*onenter)(void);
    void (*ontab)(void);
    void (*onlosefocus)(void);
};

void edit_draw(EDIT *edit, int x, int y, int width, int height);

_Bool edit_mmove(EDIT *edit, int x, int y, int width, int height, int mx, int my, int dy);
_Bool edit_mdown(EDIT *edit);
_Bool edit_dclick(EDIT *edit, _Bool triclick);
_Bool edit_mright(EDIT *edit);
_Bool edit_mwheel(EDIT *edit, int height, double d);
_Bool edit_mup(EDIT *edit);
_Bool edit_mleave(EDIT *edit);

void edit_press(void);

void edit_char(uint32_t ch, _Bool control, uint8_t flags);

int edit_selection(EDIT *edit, char_t *data, int len);
int edit_copy(char_t *data, int len);
void edit_paste(char_t *data, int len, _Bool select);

_Bool edit_active(void);

void edit_resetfocus(void);
void edit_setfocus(EDIT *edit);
void edit_setstr(EDIT *edit, char_t *str, uint16_t length);
