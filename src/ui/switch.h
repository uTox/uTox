#ifndef SWITCH_H
#define SWITCH_H

#include "../ui.h"
#include "svg.h"

typedef struct uiswitch UISWITCH;
struct uiswitch {
    PANEL panel;

    SVG_IMG style_outer;
    SVG_IMG style_toggle;
    SVG_IMG style_icon_off;
    SVG_IMG style_icon_on;

    // Width/height of the toggle and the icons. Used for centering.
    int toggle_w, toggle_h, icon_off_w, icon_off_h, icon_on_w, icon_on_h;

    // Background RGB color, when Idle/Hovered/Pressed respectively.
    uint32_t bg_color,  // Switch normal background color
        sw_color,       // Switch 'toggle' color
        hover_color,    // Switch mouse over color
        press_color,    // Switch mouse down color
        disabled_color; // Switch disabled bg color

    MAYBE_I18NAL_STRING tooltip_text;

    bool switch_on;
    bool mouseover, mousedown, disabled, nodraw;

    void (*onright)(void); // called when right mouse uiswitch goes down
    void (*onpress)(void);
    void (*update)(UISWITCH *s);
};

void switch_draw(UISWITCH *s, int x, int y, int width, int height);
bool switch_mmove(UISWITCH *s, int x, int y, int width, int height, int mx, int my, int dx, int dy);
bool switch_mdown(UISWITCH *s);
bool switch_mright(UISWITCH *s);
bool switch_mwheel(UISWITCH *s, int height, double d, bool smooth);
bool switch_mup(UISWITCH *s);
bool switch_mleave(UISWITCH *s);

#endif
