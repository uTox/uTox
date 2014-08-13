
typedef struct {
    int x, y, width, height;
    _Bool open;
    uint8_t count, over, down;
    void (*onselect)(uint8_t);
    uint8_t *names[8];
} CONTEXTMENU;

void contextmenu_draw(void);
_Bool contextmenu_mmove(int mx, int my, int dx, int dy);
_Bool contextmenu_mdown(void);
_Bool contextmenu_mup(void);
_Bool contextmenu_mleave(void);

void contextmenu_new(uint8_t **names, uint8_t count, void (*onselect)(uint8_t));
