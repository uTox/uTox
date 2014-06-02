
struct scrollable
{
    PANEL panel;

    double d;
    _Bool mousedown, mouseover, mouseover2;
    int content_height;
};

void scroll_draw(SCROLLABLE *s, int x, int y, int width, int height);
int scroll_gety(SCROLLABLE *s, int height);

_Bool scroll_mmove(SCROLLABLE *s, int x, int y, int dy, int width, int height);
_Bool scroll_mdown(SCROLLABLE *s);
_Bool scroll_mright(SCROLLABLE *s);
_Bool scroll_mwheel(SCROLLABLE *s, int height, double d);
_Bool scroll_mup(SCROLLABLE *s);
_Bool scroll_mleave(SCROLLABLE *s);
