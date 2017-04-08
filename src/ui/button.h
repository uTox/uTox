#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "panel.h"

#include "../ui.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct button BUTTON;
struct button {
    PANEL panel;

    /* Button bitmap id,
     * fill is top-left aligned
     * icon is centered. */
    int bm_fill, bm_icon;

    // Width & height of bm_icon. Used for centering.
    int icon_w, icon_h;

    // Background RGB color for bm picture, when Idle/Hovered/Pressed respectively.
    uint32_t c1,  // Button normal background color
             c2,  // Button hover background color
             c3,  // Button active (press) background color
             ct1, // Button contents (text or icon) color
             ct2, // Button contents (text or icon) hover color
             cd;

    MAYBE_I18NAL_STRING button_text;
    MAYBE_I18NAL_STRING tooltip_text;

    bool mouseover, mousedown, disabled, nodraw;

    void (*onright)(void); // called when right mouse button goes down
    void (*on_mdn)(void);
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


// TODO these may move
void button_setcolors_success(BUTTON *b);
void button_setcolors_danger(BUTTON *b);
void button_setcolors_warning(BUTTON *b);
void button_setcolors_disabled(BUTTON *b);


#endif // UI_BUTTON_H
