#ifndef DRAW_HELPERS_H
#define DRAW_HELPERS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct native_image NATIVE_IMAGE;
typedef struct panel PANEL;

void draw_avatar_image(NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t targetwidth,
                       uint32_t targetheight);

/* Top left self interface Avatar, name, statusmsg, status icon */
void draw_user_badge(int x, int y, int width, int height);
void draw_splash_page(int x, int y, int w, int h);

/* Header for friend chat window */
void draw_friend(int x, int y, int w, int height);

void draw_group(int x, int y, int w, int height);
/* Draw an invite to be a friend window */
void draw_friend_request(int x, int y, int w, int height);
/* Draw add a friend window */
void draw_add_friend(int x, int y, int w, int height);
/* Draw the text for profile password window */
void draw_profile_password(int x, int y, int w, int height);

/* Top bar for user settings */
void draw_settings_header(int x, int y, int w, int height);

/* TODO make this fxn readable */
void draw_settings_sub_header(int x, int y, int w, int height);


/* draw switch profile top bar */
/* Text content for settings page */
void draw_settings_text_profile(int x, int y, int w, int h);

void draw_settings_text_devices(int x, int y, int w, int h);
void draw_settings_text_password(int x, int y, int w, int h);

void draw_settings_text_network(int x, int y, int w, int height);

void draw_settings_text_ui(int x, int y, int w, int height);

void draw_settings_text_av(int x, int y, int w, int height);

void draw_settings_text_adv(int x, int y, int w, int height);

void draw_settings_text_notifications(int x, int y, int w, int height);

void draw_friend_settings(int x, int y, int width, int height);

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
