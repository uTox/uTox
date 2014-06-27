typedef struct
{
    uint8_t *name;
    void *handle;
}DROP_ELEMENT;

struct dropdown {
    PANEL panel;
    _Bool mouseover, open;
    uint16_t dropcount, selected, over;
    DROP_ELEMENT *drop;
    void (*onselect)(void*);
};

void dropdown_drawactive(void);

void dropdown_draw(DROPDOWN *b, int x, int y, int width, int height);
_Bool dropdown_mmove(DROPDOWN *b, int x, int y, int width, int height, int mx, int my, int dyt);
_Bool dropdown_mdown(DROPDOWN *b);
_Bool dropdown_mright(DROPDOWN *b);
_Bool dropdown_mwheel(DROPDOWN *b, int height, double d);
_Bool dropdown_mup(DROPDOWN *b);
_Bool dropdown_mleave(DROPDOWN *b);

void dropdown_add(DROPDOWN *b, uint8_t *name, void *handle);
