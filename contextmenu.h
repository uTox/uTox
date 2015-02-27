
typedef struct contextmenu {
    int x, y, width, height;
    _Bool open;
    uint8_t count, over, down;
    void (*onselect)(uint8_t);
    STRING* (*ondisplay)(uint8_t, const struct contextmenu*);
    void *userdata;
} CONTEXTMENU;

void contextmenu_draw_common(int target);
_Bool contextmenu_mmove(int mx, int my, int dx, int dy);
_Bool contextmenu_mdown(int target);
_Bool contextmenu_mup(int target);
_Bool contextmenu_mleave(void);

void contextmenu_new(uint8_t count, UI_STRING_ID* menu_string_ids, void (*onselect)(uint8_t));
void contextmenu_new_ex(uint8_t count, void *userdata, void (*onselect)(uint8_t), STRING* (*ondisplay)(uint8_t, const CONTEXTMENU*));
