#ifndef DRAW_HELPERS_H
#define DRAW_HELPERS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct native_image NATIVE_IMAGE;
typedef struct panel PANEL;


void draw_notification(int x, int y, int w, int h);

void draw_avatar_image(NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t targetwidth,
                       uint32_t targetheight);

/* Top left self interface Avatar, name, statusmsg, status icon */
void draw_user_badge(int x, int y, int width, int height);
void draw_splash_page(int x, int y, int w, int h);

void draw_group(int x, int y, int w, int height);


void draw_group_settings(int x, int y, int width, int height);

void draw_background(int x, int y, int width, int height);

void draw_nospam_settings(int x, int y, int w, int h);

/* These remain for legacy reasons, PANEL_MAIN calls these by default when not given it's own function to call */

void background_draw(PANEL *p, int x, int y, int width, int height);
bool background_mmove(PANEL *p, int x, int y, int width, int height, int mx, int my, int dx, int dy);
bool background_mdown(PANEL *p);
bool background_mright(PANEL *p);
bool background_mwheel(PANEL *p, int height, double d, bool smooth);
bool background_mup(PANEL *p);
bool background_mleave(PANEL *p);

#endif
