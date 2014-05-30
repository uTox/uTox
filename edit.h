/*todo: replace windows functions, multiline edits, add missing edit functions (ex: double click to select word)*/

typedef struct {
    _Bool multiline, mouseover;
    uint16_t flags, length, maxlength;
    int x, y, bottom, right;
    uint8_t *data;
    void (*onenter)(void);
    void (*onredraw)(void);
} EDIT;

void edit_mousemove(EDIT *edit, int x, int y);
void edit_mousedown(EDIT *edit, int x, int y);
void edit_mouseup(EDIT *edit);
void edit_mouseleave(EDIT *edit);
void edit_rightclick(EDIT *edit, int x, int y);

void edit_draw(EDIT *edit);

void edit_char(uint32_t ch);

void edit_cut(void);
void edit_copy(void);
void edit_paste(void);
void edit_delete(void);
void edit_selectall(void);
void edit_clear(void);

_Bool edit_active(void);

void edit_setfocus(EDIT *edit);
void edit_setstr(EDIT *edit, uint8_t *str, uint16_t length);
