#include "main.h"

#include "ui_strings.h"
#include "ui_edits.h"
#include "ui_buttons.h"

uint32_t status_color[] = {
    C_GREEN,
    C_YELLOW,
    C_RED,
    C_RED
};

static void drawself(void)
{
    //40x40 self icon at 10,10
    setcolor(WHITE);
    setfont(FONT_SELF_NAME);
    drawtextrange(SELF_NAME_X, SELF_STATUS_X, SELF_NAME_Y, self.name, self.name_length);

    setcolor(C_STATUS);
    setfont(FONT_STATUS);
    drawtextrange(SELF_MSG_X, SELF_STATUS_X, SELF_MSG_Y, self.statusmsg, self.statusmsg_length);

    drawalpha(BM_CONTACT, SELF_AVATAR_X, SELF_AVATAR_Y, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, WHITE);

    drawalpha(BM_STATUSAREA, SELF_STATUS_X, SELF_STATUS_Y, BM_STATUSAREA_WIDTH, BM_STATUSAREA_HEIGHT, LIST_MAIN);

    uint8_t status = tox_connected ? self.status : 3;
    drawalpha(BM_ONLINE + status, SELF_STATUS_X + BM_STATUSAREA_WIDTH / 2 - BM_STATUS_WIDTH / 2, SELF_STATUS_Y + BM_STATUSAREA_HEIGHT / 2 - BM_STATUS_WIDTH / 2, BM_STATUS_WIDTH, BM_STATUS_WIDTH, status_color[status]);
}

static void drawfriend(int x, int y, int w, int height)
{
    FRIEND *f = sitem->data;

    drawalpha(BM_CONTACT, LIST_RIGHT + SCALE * 5, SCALE * 5, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, LIST_MAIN);

    setcolor(C_TITLE);
    setfont(FONT_TITLE);
    drawtextrange(LIST_RIGHT + 30 * SCALE, width - 62 * SCALE, 9 * SCALE, f->name, f->name_length);

    setcolor(LIST_MAIN);
    setfont(FONT_STATUS);
    drawtextrange(LIST_RIGHT + 30 * SCALE, width - 62 * SCALE, 16 * SCALE, f->status_message, f->status_length);

    if(!f->calling) {
        button_call.c1 = C_GREEN;
        button_call.c2 = C_GREEN_LIGHT;
        button_call.c3 = C_GREEN_LIGHT;
        button_video.c1 = C_GREEN;
        button_video.c2 = C_GREEN_LIGHT;
        button_video.c3 = C_GREEN_LIGHT;

        return;
    }

    button_call.c1 = C_GRAY;
    button_call.c2 = C_GRAY;
    button_call.c3 = C_GRAY;
    button_video.c1 = C_GRAY;
    button_video.c2 = C_GRAY;
    button_video.c3 = C_GRAY;

    BUTTON *b;
    if(f->calling & 4) {
        b = &button_video;
    } else {
        b = &button_call;
    }

    switch(f->calling & 3) {
    case CALL_INVITED: {
        b->c1 = C_YELLOW;
        b->c2 = C_YELLOW_LIGHT;
        break;
    }

    case CALL_RINGING: {
        b->c1 = C_GRAY;
        b->c2 = C_GRAY;
        break;
    }

    case CALL_OK: {
        b->c1 = C_RED;
        b->c2 = C_RED_LIGHT;
        break;
    }
    }
}

static void drawgroup(int x, int y, int w, int height)
{
    GROUPCHAT *g = sitem->data;

    drawalpha(BM_GROUP, LIST_RIGHT + SCALE * 5, SCALE * 5, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, LIST_MAIN);

    setcolor(C_TITLE);
    setfont(FONT_TITLE);
    drawtext(LIST_RIGHT + 30 * SCALE, 5 * SCALE, g->name, g->name_length);

    setcolor(LIST_MAIN);
    setfont(FONT_STATUS);
    drawtext(LIST_RIGHT + 30 * SCALE, 12 * SCALE, g->topic, g->topic_length);


    setcolor(GRAY(150));
    int i = 0, j = 0, k = LIST_RIGHT + 30 * SCALE;
    while(i < g->peers)
    {
        uint8_t *name = g->peername[j];
        if(name)
        {
            uint8_t buf[134];
            memcpy(buf, name + 1, name[0]);
            memcpy(buf + name[0], ", ", 2);

            int w = textwidth(buf, name[0] + 2);
            if(k + w >= width) {
                drawstr(k, 18 * SCALE, "...");
                break;
            }

            drawtext(k, 18 * SCALE, buf, name[0] + 2);

            k += w;
            i++;
        }
        j++;
    }
}

static void drawfriendreq(int x, int y, int width, int height)
{
    setcolor(C_TITLE);
    setfont(FONT_SELF_NAME);
    drawstr(LIST_RIGHT + SCALE * 5, SCALE * 10, "Friend Request");
}

static void drawadd(int x, int y, int width, int height)
{
    setcolor(C_TITLE);
    setfont(FONT_SELF_NAME);
    drawstr(LIST_RIGHT + SCALE * 5, SCALE * 10, "Add Friends");
}


static void drawsettings(int x, int y, int width, int height)
{
    setcolor(C_TITLE);
    setfont(FONT_SELF_NAME);
    drawstr(LIST_RIGHT + SCALE * 5, SCALE * 10, "User Settings");
}

static void drawadd_content(int x, int y, int width, int height)
{
    setcolor(C_TITLE);
    setfont(FONT_TEXT);
    drawstr(LIST_RIGHT + SCALE * 5, y + SCALE * 5, "Tox ID");

    drawstr(LIST_RIGHT + SCALE * 5, y + SCALE * 29, "Message");

    if(addfriend_status) {
        setfont(FONT_MISC);
        setcolor(C_RED);
        drawtext(LIST_RIGHT + SCALE * 5, y + SCALE * 83, addstatus[addfriend_status - 1].str, addstatus[addfriend_status - 1].length);
    }
}


static void drawsettings_content(int x, int y, int w, int height)
{
    setcolor(C_TITLE);
    setfont(FONT_TEXT);
    drawstr(LIST_RIGHT + SCALE * 5, y + SCALE * 5, "Name");

    drawstr(LIST_RIGHT + SCALE * 5, y + SCALE * 29, "Status Message");

    drawtextrange(LIST_RIGHT + SCALE * 5, width - SCALE * 5, y + SCALE * 64, self.id, sizeof(self.id));

    setfont(FONT_SELF_NAME);

    drawstr(LIST_RIGHT + SCALE * 5, y + SCALE * 54, "Tox ID");

}

static void background_draw(PANEL *p, int x, int y, int width, int height)
{
    drawrect(0, 0, LIST_RIGHT, LIST_Y - 1, LIST_DARK);
    drawhline(0, LIST_Y - 1, LIST_RIGHT, LIST_EDGE);
    drawrect(0, LIST_Y, LIST_RIGHT, height + LIST_BOTTOM, LIST_MAIN);
    drawrect(0, height + LIST_BOTTOM, LIST_RIGHT, height - 1, LIST_DARK);
    drawhline(0, height - 1, LIST_RIGHT, LIST_EDGE2);

    drawself();

    drawrect(LIST_RIGHT, 0, width, height, WHITE);

    drawvline(LIST_RIGHT, 1, LIST_Y - 1, LIST_EDGE3);
    drawpixel(LIST_RIGHT, LIST_Y - 1, LIST_EDGE2);
    drawvline(LIST_RIGHT, LIST_Y, height - SCALE * 15, LIST_EDGE4);
    drawpixel(LIST_RIGHT, height - SCALE * 15, LIST_EDGE5);

    drawhline(LIST_RIGHT + 1, LIST_Y - 1, width, C_GRAY);
}

static _Bool background_mmove(PANEL *p, int x, int y, int dy, int width, int height)
{
    return 0;
}

static _Bool background_mdown(PANEL *p)
{
    return 0;
}

static _Bool background_mright(PANEL *p)
{
    return 0;
}

static _Bool background_mwheel(PANEL *p, int height, double d)
{
    return 0;
}

static _Bool background_mup(PANEL *p)
{
    return 0;
}

static _Bool background_mleave(PANEL *p)
{
    return 0;
}

SCROLLABLE scroll_list = {
    .panel = {
        .type = PANEL_SCROLLABLE,
        .x = 0,
        .y = LIST_Y,
        .width = LIST_RIGHT + 1,
        .height = LIST_BOTTOM,
    },

    .color = LIST_DARK,
    .x = 2,
    .left = 1,
},

scroll_friend = {
    .panel = {
        .type = PANEL_SCROLLABLE,
        .y = LIST_Y,
        .height = MESSAGES_BOTTOM,
    },

    .color = C_SCROLL,
},

scroll_group = {
    .panel = {
        .type = PANEL_SCROLLABLE,
        .y = LIST_Y,
        .height = MESSAGES_BOTTOM,
    },

    .color = C_SCROLL,
},

scroll_add = {
    .panel = {
        .y = LIST_Y,
        .type = PANEL_SCROLLABLE,
    },
    .content_height = 200 * SCALE,
    .color = C_SCROLL,
},

scroll_settings = {
    .panel = {
        .y = LIST_Y,
        .type = PANEL_SCROLLABLE,
    },
    .content_height = 200 * SCALE,
    .color = C_SCROLL,
};

/* */
MESSAGES messages_friend = {
    .panel = {
        .type = PANEL_MESSAGES,
        .y = LIST_Y,
        .height = MESSAGES_BOTTOM,
        .width = -SCROLL_WIDTH,
        .content_scroll = &scroll_friend,
    }
},

messages_group = {
    .panel = {
        .type = PANEL_MESSAGES,
        .y = LIST_Y,
        .height = MESSAGES_BOTTOM,
        .width = -SCROLL_WIDTH,
        .content_scroll = &scroll_group,
    },
    .type = 1
};

PANEL panel_list = {
    .type = PANEL_LIST,
    .x = 0,
    .y = LIST_Y,
    .width = LIST_RIGHT + 1,
    .height = LIST_BOTTOM,
    .content_scroll = &scroll_list,
},

panel_add = {
    .y = LIST_Y,
    .drawfunc = drawadd_content,
    .content_scroll = &scroll_add,
    .child = (PANEL*[]) {
            (void*)&button_addfriend,
            (void*)&edit_addid, (void*)&edit_addmsg,

            NULL
        }
},

panel_settings = {
    .y = LIST_Y,
    .drawfunc = drawsettings_content,
    .content_scroll = &scroll_settings,
    .child = (PANEL*[]) {
            (void*)&button_copyid,
            (void*)&edit_name, (void*)&edit_status,
            NULL
        }
},


panel_item[] = {
    {
        .type = PANEL_NONE,
        //.disabled = 1,
        .drawfunc = drawadd,
        .child = (PANEL*[]) {
            (void*)&scroll_add,
            &panel_add,
            NULL
        }
    },

    {
        .type = PANEL_NONE,
        .disabled = 1,
        .drawfunc = drawsettings,
        .child = (PANEL*[]) {
            (void*)&scroll_settings,
            &panel_settings,
            NULL
        }
    },

    {
        .type = PANEL_NONE,
        .disabled = 1,
        .drawfunc = drawfriend,
        .child = (PANEL*[]) {
            (void*)&button_call, (void*)&button_video, (void*)&button_sendfile,
            (void*)&edit_msg,
            (void*)&scroll_friend,
            (void*)&messages_friend,
            NULL
        }
    },

    {
        .type = PANEL_NONE,
        .disabled = 1,
        .drawfunc = drawgroup,
        .child = (PANEL*[]) {
            (void*)&edit_msg,
            (void*)&scroll_group,
            (void*)&messages_group,
            NULL
        }
    },

    {
        .type = PANEL_NONE,
        .disabled = 1,
        .drawfunc = drawfriendreq,
        .child = (PANEL*[]) {
            (void*)&button_acceptfriend,
            NULL
        }
    },
},

panel_side = {
    .type = PANEL_NONE,
    .x = LIST_RIGHT,
    .y = 0,
    .width = 0,
    .height = 0,
    .child = (PANEL*[]) {
        &panel_item[0], &panel_item[1], &panel_item[2], &panel_item[3], &panel_item[4], NULL
    }
},

panel_main = {
    .type = PANEL_MAIN,
    .x = 0,
    .y = 0,
    .width = 0,
    .height = 0,
    .child = (PANEL*[]) {
        &panel_list, &panel_side,
        (void*)&scroll_list,
        (void*)&button_add, (void*)&button_groups, (void*)&button_transfer, (void*)&button_settings,
        NULL
    }
};

#define FUNC(x, ret, ...) static ret (* x##func[])(void *p, ##__VA_ARGS__) = { \
    (void*)background_##x, \
    (void*)messages_##x, \
    (void*)list_##x, \
    (void*)button_##x, \
    (void*)edit_##x, \
    (void*)scroll_##x, \
};

FUNC(draw, void, int x, int y, int width, int height);
FUNC(mmove, _Bool, int x, int y, int dy, int width, int height);
FUNC(mdown, _Bool);
FUNC(mright, _Bool);
FUNC(mwheel, _Bool, int height, double d);
FUNC(mup, _Bool);
FUNC(mleave, _Bool);

#undef FUNC
#define FUNC() {\
    int dx = (p->x < 0) ? width + p->x : p->x;\
    int dy = (p->y < 0) ? height + p->y : p->y;\
    x += dx; \
    y += dy; \
    width = (p->width <= 0) ? width + p->width - dx : p->width; \
    height = (p->height <= 0) ? height + p->height - dy : p->height; }\

static void panel_draw_sub(PANEL *p, int x, int y, int width, int height)
{
    FUNC();

    if(p->content_scroll) {
        pushclip(x, y, width, height);

        y -= scroll_gety(p->content_scroll, height);
    }


    if(p->type) {
        drawfunc[p->type - 1](p, x, y, width, height);
    } else {
        if(p->drawfunc) {
            p->drawfunc(x, y, width, height);
        }
    }

    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                panel_draw_sub(subp, x, y, width, height);
            }
        }
    }

    if(p->content_scroll) {
        popclip();
    }
}

void panel_draw(PANEL *p, int x, int y, int width, int height)
{
    FUNC();

    pushclip(x, y, width, height);

    if(p->type) {
        drawfunc[p->type - 1](p, x, y, width, height);
    } else {
        if(p->drawfunc) {
            p->drawfunc(x, y, width, height);
        }
    }

    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                panel_draw_sub(subp, x, y, width, height);
            }
        }
    }

    popclip();

    enddraw(x, y, width, height);
}

void panel_update(PANEL *p, int x, int y, int width, int height)
{
    FUNC();

    if(p->type == PANEL_MESSAGES) {
        MESSAGES *m = (void*)p;
        m->width = width;
        if(!p->disabled) {
            messages_updateheight(m);
        }
    }

    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            panel_update(subp, x, y, width, height);
        }
    }
}

_Bool panel_mmove(PANEL *p, int x, int y, int width, int height, int mx, int my, int dy)
{
    mx -= (p->x < 0) ? width + p->x : p->x;
    my -= (p->y < 0) ? height + p->y : p->y;
    FUNC();

    if(p->content_scroll) {
        int dy = scroll_gety(p->content_scroll, height);
        y -= dy;
        my += dy;
    }

    _Bool draw = p->type ? mmovefunc[p->type - 1](p, mx, my, dy, width, height) : 0;
    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                draw |= panel_mmove(subp, x, y, width, height, mx, my, dy);
            }
        }
    }

    if(draw && p == &panel_main) {
        redraw();
    }

    return draw;
}

static _Bool panel_mdown_sub(PANEL *p)
{
    if(p->type && mdownfunc[p->type - 1](p)) {
        return 1;
    }

    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                if(panel_mdown_sub(subp)) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

void panel_mdown(PANEL *p)
{
    _Bool draw = edit_active();
    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                if(panel_mdown_sub(subp)) {
                    draw = 1;
                    break;
                }
            }
        }
    }

    if(draw) {
        redraw();
    }
}

_Bool panel_dclick(PANEL *p, _Bool triclick)
{
    _Bool draw = 0;
    if(p->type == PANEL_EDIT) {
        draw = edit_dclick((EDIT*)p, triclick);
    } else if(p->type == PANEL_MESSAGES) {
        draw = messages_dclick((MESSAGES*)p, triclick);
    }

    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                draw = panel_dclick(subp, triclick);
                if(draw) {
                    break;
                }
            }
        }
    }

    if(draw && p == &panel_main) {
        redraw();
    }

    return draw;
}

_Bool panel_mright(PANEL *p)
{
    _Bool draw = p->type ? mrightfunc[p->type - 1](p) : 0;
    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                draw |= panel_mright(subp);
            }
        }
    }

    if(draw && p == &panel_main) {
        redraw();
    }

    return draw;
}

_Bool panel_mwheel(PANEL *p, int x, int y, int width, int height, double d)
{
    FUNC();

    _Bool draw = p->type ? mwheelfunc[p->type - 1](p, height, d) : 0;
    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                draw |= panel_mwheel(subp, x, y, width, height, d);
            }
        }
    }

    if(draw && p == &panel_main) {
        redraw();
    }

    return draw;
}

_Bool panel_mup(PANEL *p)
{
    _Bool draw = p->type ? mupfunc[p->type - 1](p) : 0;
    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                draw |= panel_mup(subp);
            }
        }
    }

    if(draw && p == &panel_main) {
        redraw();
    }

    return draw;
}

_Bool panel_mleave(PANEL *p)
{
    _Bool draw = p->type ? mleavefunc[p->type - 1](p) : 0;
    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                draw |= panel_mleave(subp);
            }
        }
    }

    if(draw && p == &panel_main) {
        redraw();
    }

    return draw;
}
