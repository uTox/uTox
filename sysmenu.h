/* todo: colors (use colors.h new defines!), remove windows-specific functions, use SYSMENU struct */

typedef struct
{
    PANEL panel;
    _Bool mdown;
    uint8_t active, mover;
}SYSMENU;

void sysmenu_draw(SYSMENU *s, int x, int y, int width, int height);

_Bool sysmenu_mmove(SYSMENU *s, int x, int y, int dy, int width, int height);
_Bool sysmenu_mdown(SYSMENU *s);
_Bool sysmenu_mright(SYSMENU *s);
_Bool sysmenu_mwheel(SYSMENU *s, int height, double d);
_Bool sysmenu_mup(SYSMENU *s);
_Bool sysmenu_mleave(SYSMENU *s);
