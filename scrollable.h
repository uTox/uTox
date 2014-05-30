
typedef struct
{
    double d;
    _Bool mousedown, mouseover;
    int x, y, width, height;
    int content_height;
    void (*onscroll)(void);
}SCROLLABLE;

void scroll_draw(SCROLLABLE *s);
int scroll_gety(SCROLLABLE *s);
void scroll_mousemove(SCROLLABLE *s, int x, int y, int dy);
void scroll_mousedown(SCROLLABLE *s);
void scroll_mouseup(SCROLLABLE *s);
void scroll_mouseleave(SCROLLABLE *s);
void scroll_mousewheel(SCROLLABLE *s, int x, int y, double d);

#define scrolls(x) ((x)->content_height > (x)->height)
