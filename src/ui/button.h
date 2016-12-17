#ifndef BUTTON_H
#define BUTTON_H

#include "../ui.h"

#include <stdbool.h>

typedef struct button BUTTON;
struct button {
    PANEL panel;

    // Button picture id, top-left aligned/centered respectively.
    int bm, bm2;

    // Width/height of bm2 picture. Used for centering.
    int bw, bh;

    // Background RGB color for bm picture, when Idle/Hovered/Pressed respectively.
    uint32_t c1, // Button normal background colour
        c2,      // Button hover background colour
        c3,      // Button active (press) background colour
        ct1,     // Button contents (text or icon) colour
        ct2,     // Button contents (text or icon) hover colour
        cd;

    MAYBE_I18NAL_STRING button_text;
    MAYBE_I18NAL_STRING tooltip_text;

    bool mouseover, mousedown, disabled, nodraw;

    void (*onright)(void); // called when right mouse button goes down
    void (*on_mup)(void);
    void (*update)(BUTTON *b);
};

void button_draw(BUTTON *b, int x, int y, int width, int height);
bool button_mmove(BUTTON *b, int x, int y, int width, int height, int mx, int my, int dx, int dy);

bool button_mdown(BUTTON *b);
bool button_mup(BUTTON *b);

bool button_mright(BUTTON *b);
bool button_mwheel(BUTTON *b, int height, double d, bool smooth);

bool button_mleave(BUTTON *b);

#endif
