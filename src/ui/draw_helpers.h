// draw_helpers.h
#ifndef DRAW_HELPERS_H
#define DRAW_HELPERS_H

#include "../main.h"
#include "../ui.h"


void draw_avatar_image(NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t targetwidth,
                       uint32_t targetheight);

/* Top left self interface Avatar, name, statusmsg, status icon */
void draw_user_badge(int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height));
void draw_splash_page(int x, int y, int w, int h);

/* Header for friend chat window */
void draw_friend(int x, int y, int w, int height);

void draw_group(int UNUSED(x), int UNUSED(y), int UNUSED(w), int UNUSED(height));
/* Draw an invite to be a friend window */
void draw_friend_request(int UNUSED(x), int UNUSED(y), int UNUSED(w), int UNUSED(height));
/* Draw add a friend window */
void draw_add_friend(int UNUSED(x), int UNUSED(y), int UNUSED(w), int height);
/* Draw the text for profile password window */
void draw_profile_password(int UNUSED(x), int UNUSED(y), int UNUSED(w), int height);

/* Top bar for user settings */
void draw_settings_header(int UNUSED(x), int UNUSED(y), int w, int UNUSED(height));

/* TODO make this fxn readable */
void draw_settings_sub_header(int x, int y, int w, int UNUSED(height));


/* draw switch profile top bar */
/* Text content for settings page */
void draw_settings_text_profile(int x, int y, int w, int h);

void draw_settings_text_devices(int x, int y, int w, int h);
void draw_settings_text_password(int x, int y, int w, int h);

void draw_settings_text_network(int x, int y, int w, int UNUSED(height));

void draw_settings_text_ui(int x, int y, int w, int UNUSED(height));

void draw_settings_text_av(int x, int y, int w, int UNUSED(height));

void draw_settings_text_adv(int UNUSED(x), int y, int UNUSED(w), int UNUSED(height));

void draw_settings_text_notifications(int UNUSED(x), int y, int UNUSED(w), int UNUSED(height));

void draw_friend_settings(int UNUSED(x), int y, int width, int height);

void draw_group_settings(int UNUSED(x), int y, int width, int height);

void draw_background(int UNUSED(x), int UNUSED(y), int width, int height);

/* These remain for legacy reasons, PANEL_MAIN calls these by default when not given it's own function to call */
void background_draw(PANEL *UNUSED(p), int UNUSED(x), int UNUSED(y), int width, int height);
bool background_mmove(PANEL *UNUSED(p), int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height),
                      int UNUSED(mx), int UNUSED(my), int UNUSED(dx), int UNUSED(dy));
bool background_mdown(PANEL *UNUSED(p));
bool background_mright(PANEL *UNUSED(p));
bool background_mwheel(PANEL *UNUSED(p), int UNUSED(height), double UNUSED(d), bool UNUSED(smooth));
bool background_mup(PANEL *UNUSED(p));
bool background_mleave(PANEL *UNUSED(p));

#endif
