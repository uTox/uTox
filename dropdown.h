// userdata of list-based dropdown consists of these records
typedef struct
{
    MAYBE_I18NAL_STRING name;
    void *handle;
}DROP_ELEMENT;

typedef struct dropdown {
    PANEL panel;
    _Bool mouseover, open;
    uint16_t dropcount, selected, over;
    void (*onselect)(uint16_t, const struct dropdown*);
    STRING* (*ondisplay)(uint16_t, const struct dropdown*);
    void *userdata;
} DROPDOWN;

void dropdown_drawactive_common(int target);

void dropdown_draw_common(DROPDOWN *b, int target, int x, int y, int width, int height);
_Bool dropdown_mmove(DROPDOWN *b, int target, int x, int y, int width, int height, int mx, int my, int dx, int dy);
_Bool dropdown_mdown(DROPDOWN *b, int target);
_Bool dropdown_mright(DROPDOWN *b);
_Bool dropdown_mwheel(DROPDOWN *b, int height, double d);
_Bool dropdown_mup(DROPDOWN *b, int target);
_Bool dropdown_mleave(DROPDOWN *b);

STRING* simple_dropdown_ondisplay(uint16_t, const DROPDOWN*);

STRING* list_dropdown_ondisplay(uint16_t, const DROPDOWN*);
void list_dropdown_add_hardcoded(DROPDOWN*, uint8_t* name, void *handle);
void list_dropdown_add_localized(DROPDOWN*, UI_STRING_ID string_id, void *handle);
void list_dropdown_clear(DROPDOWN*);
