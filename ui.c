#include "main.h"

#include "icons/contact.c"
#include "icons/group.c"
#include "icons/misc.c"

/* edits */
static uint8_t edit_name_data[128], edit_status_data[128], edit_addid_data[TOX_FRIEND_ADDRESS_SIZE * 2], edit_addmsg_data[1024], edit_msg_data[1024];

static void edit_name_onenter(void)
{
    uint8_t *data = edit_name_data;
    uint16_t length = edit_name.length;

    if(!length) {
        return;
    }

    memcpy(self.name, data, length);
    self.name_length = length;

    tox_postmessage(TOX_SETNAME, length, 0, self.name);//!

    panel_redraw(NULL);//list
}

static void edit_status_onenter(void)
{
    uint8_t *data = edit_status_data;
    uint16_t length = edit_status.length;

    if(!length) {
        return;
    }

    void *p = realloc(self.statusmsg, length);
    if(!p) {
        return;
    }

    self.statusmsg = p;
    memcpy(self.statusmsg, data, length);
    self.statusmsg_length = length;

    tox_postmessage(TOX_SETSTATUSMSG, length, 0, self.statusmsg);//!

    panel_redraw(NULL);//list

}

static void edit_msg_onenter(void)
{
    uint16_t length = edit_msg.length;
    if(length == 0) {
        return;
    }

    if(sitem->item == ITEM_FRIEND) {
        FRIEND *f = sitem->data;

        uint16_t *data = malloc(length + 4);
        data[0] = 0;
        data[1] = length;
        memcpy((void*)data + 4, edit_msg_data, length);

        f->message = realloc(f->message, (f->msg + 1) * sizeof(void*));
        f->message[f->msg++] = data;

        //
        void *d = malloc(length);
        memcpy(d, edit_msg_data, length);

        tox_postmessage(TOX_SENDMESSAGE, (f - friend), length, d);
    } else {
        GROUPCHAT *g = sitem->data;

        void *d = malloc(length);
        memcpy(d, edit_msg_data, length);

        tox_postmessage(TOX_SENDMESSAGEGROUP, (g - group), length, d);
    }

    edit_clear();

    redraw();
}

#define EDIT_NAME_Y (MAIN_Y + 64)
#define EDIT_STATUS_Y (MAIN_Y + 114)
#define EDIT_ADDID_Y (MAIN_Y + 64)
#define EDIT_ADDMSG_Y (MAIN_Y + 114)

EDIT edit_name = {
    .panel = {
        .type = PANEL_EDIT,
        .x = 0,
        .y = 64,
        .height = 24,
        .width = 0
    },

    .multiline = 0,
    .maxlength = 128,
    .data = edit_name_data,
    .onenter = edit_name_onenter,
},

edit_status = {
    .panel = {
        .type = PANEL_EDIT,
        .x = 0,
        .y = 114,
        .height = 24,
        .width = 0
    },
    .multiline = 0,
    .maxlength = 128,
    .data = edit_status_data,
    .onenter = edit_status_onenter,
},

edit_addid = {
    .panel = {
        .type = PANEL_EDIT,
        .x = 0,
        .y = 64,
        .height = 24,
        .width = 0
    },
    .multiline = 0,
    .maxlength = sizeof(edit_addid_data),
    .data = edit_addid_data,
},

edit_addmsg = {
    .panel = {
        .type = PANEL_EDIT,
        .x = 0,
        .y = 114,
        .height = 84,
        .width = 0
    },
    .multiline = 0,//1,
    .maxlength = sizeof(edit_addmsg_data),
    .data = edit_addmsg_data,
},

edit_msg = {
    .panel = {
        .type = PANEL_EDIT,
        .x = 0,
        .y = -84,
        .height = 84,
        .width = 0
    },
    .multiline = 0,//1,
    .maxlength = sizeof(edit_msg_data),
    .data = edit_msg_data,
    .onenter = edit_msg_onenter,
};

/* buttons */

#define BUTTON_TEXT(x) .text = (uint8_t*)x, .text_length = sizeof(x) - 1

static void button_copyid_onpress(void)
{
    address_to_clipboard();
}

static void button_addfriend_onpress(void)
{
    friend_add(edit_addid_data, edit_addid.length, edit_addmsg.data, edit_addmsg.length);
    edit_resetfocus();
}

static void button_newgroup_onpress(void)
{
    tox_postmessage(TOX_NEWGROUP, 0, 0, NULL);
}

static void button_call_onpress(void)
{
    FRIEND *f = sitem->data;

    switch(f->calling) {
    case 0: {
        tox_postmessage(TOX_CALL, f - friend, 0, NULL);
        debug("Calling friend: %u\n", f - friend);
        break;
    }

    case 1: {
        tox_postmessage(TOX_ACCEPTCALL, f->callid, 0, NULL);
        debug("Accept Call: %u\n", f->callid);
        break;
    }

    case 2: {
        //tox_postmessage(TOX_CALL, f - friend, 0, NULL);
        //debug("Calling friend: %u\n", f - friend);
        break;
    }

    case 3: {
        tox_postmessage(TOX_HANGUP, f->callid, 0, NULL);
        debug("Ending call: %u\n", f->callid);
        break;
    }
    }
}

static void button_acceptfriend_onpress(void)
{
    FRIENDREQ *req = sitem->data;
    tox_postmessage(TOX_ACCEPTFRIEND, 0, 0, req);
}

BUTTON button_copyid = {
    .panel = {
        .type = PANEL_BUTTON,
        .x = 0,
        .y = 185,
        .width = 150,
        .height = 18,
    },
    .onpress = button_copyid_onpress,
               BUTTON_TEXT("copy to clipboard")
           },

button_addfriend = {
    .panel = {
        .type = PANEL_BUTTON,
        .x = -50,
        .y = 222,
        .width = 50,
        .height = 18,
    },
    .onpress = button_addfriend_onpress,
               BUTTON_TEXT("add")
           },

button_newgroup = {
    .panel = {
        .type = PANEL_BUTTON,
        .x = 0,
        .y = 300,
        .width = 50,
        .height = 18,
    },
    .onpress = button_newgroup_onpress,
               BUTTON_TEXT("add")
           },

button_call = {
    .panel = {
        .type = PANEL_BUTTON,
        .x = -50,
        .y = 0,
        .width = 50,
        .height = 18,
    },
    .onpress = button_call_onpress,
               BUTTON_TEXT("call")
           },

button_acceptfriend = {
    .panel = {
        .type = PANEL_BUTTON,
        .x = 0,
        .y = 40,
        .width = 50,
        .height = 18,
    },
    .onpress = button_acceptfriend_onpress,
               BUTTON_TEXT("add")
           };


SCROLLABLE scroll_list = {
},

scroll_self = {
    .content_height = 610 - (MAIN_Y + 40),
},

scroll_add = {
    .content_height = 400,
};

#define STRLEN(x) (sizeof(x) - 1)

static char *addfriend_status_str[] = {
    "Friend request sent",
    "Attempting to resolve name...",
    "Error: Invalid Tox ID",
    "Error: Message too long",
    "Error: Empty message",
    "Error: ID is self",
    "Error: Friend request already sent",
    "Error: Unknown",
    "Error: Bad ID checksum",
    "Error: Bad ID nospam",
    "Error: No memory"
};

static uint16_t addfriend_status_length[] = {
    STRLEN("Friend request sent"),
    STRLEN("Attempting to resolve name..."),
    STRLEN("Error: Invalid Tox ID"),
    STRLEN("Error: Message too long"),
    STRLEN("Error: Empty message"),
    STRLEN("Error: ID is self"),
    STRLEN("Error: Friend request already sent"),
    STRLEN("Error: Unknown"),
    STRLEN("Error: Bad ID checksum"),
    STRLEN("Error: Bad ID nospam"),
    STRLEN("Error: No memory")
};

static void background_draw(PANEL *p, int x, int y, int width, int height)
{
    RECT r = {1, 1, width - 1, height - 1};
    RECT window = {0, 0, width, height};

    framerect(&window, COLOR_BORDER);
    fillrect(&r, COLOR_BG);

    drawbitmap(BM_CORNER, width - 10, height - 10, 8, 8);

    drawhline(LIST_X, SCROLL_BOTTOM + 1, LIST_X + ITEM_WIDTH + 3, INNER_BORDER);
    drawvline(LIST_X + ITEM_WIDTH + 3, LIST_Y, SCROLL_BOTTOM + 2, INNER_BORDER);

    drawhline(MAIN_X - 9, SCROLL_BOTTOM + 1, width - 12, INNER_BORDER);
    drawvline(MAIN_X - 10, MAIN_Y, SCROLL_BOTTOM + 2, INNER_BORDER);
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

static void drawadd(int x, int y, int width, int height)
{
    setcolor(0x333333);
    setfont(FONT_TITLE);

    drawstr(x, y + 2, "Add friends and groups");

    drawhline(x, y + 29, x + 300, INNER_BORDER);

    setcolor(0x555555);
    setfont(FONT_SUBTITLE);

    drawstr(x, y + 40, "Tox ID");
    drawstr(x, y + 90, "Message");

    setfont(FONT_TEXT_LARGE);

    if(addfriend_status) {
        drawtext(x, y + 200, addfriend_status_str[addfriend_status - 1], addfriend_status_length[addfriend_status - 1]);
    }

    /*begindraw(x, y + 40, width - 12, height - 12);

    int dy = scroll_gety(&scroll_add);

    edit_addid.y = EDIT_ADDID_Y - dy;
    edit_addid.bottom = edit_addid.y + 24;

    edit_addmsg.y = EDIT_ADDMSG_Y - dy;
    edit_addmsg.bottom = edit_addmsg.y + 84;

    button_addfriend.y = MAIN_Y + 222 - dy;
    button_newgroup.y = MAIN_Y + 300 - dy;

    y -= dy;

    drawstr(x, y + 40, "Tox ID");
    drawstr(x, y + 90, "Message");

    setfont(FONT_TEXT_LARGE);

    if(addfriend_status) {
        drawtext(x, y + 200, addfriend_status_str[addfriend_status - 1], addfriend_status_length[addfriend_status - 1]);
    }

    setfont(FONT_SUBTITLE);
    drawstr(x, y + 280, "Create Group");

    edit_draw(&edit_addid);
    edit_draw(&edit_addmsg);

    button_draw(&button_addfriend);
    button_draw(&button_newgroup);

    scroll_draw(&scroll_add);

    enddraw();*/
}


SYSMENU sysmenu = {
    .panel = {
        .type = PANEL_SYSMENU,
        .x = -91,
        .y = 1,
        .width = 90,
        .height = 26,
    }
};

PANEL panel_list = {
    .type = PANEL_LIST,
    .x = LIST_X,
    .y = 12,
    .width = ITEM_WIDTH,
    .height = -13,
    .content_scroll = &scroll_list,
    .child = (PANEL*[]) {
        (void*)&scroll_list, NULL
    }
},

panel_item[] = {
    {
        .type = PANEL_NONE,
        //.disabled = 1,
        .content_scroll = &scroll_self,
        .child = (PANEL*[]) {
            //(void*)&text_name, (void*)&text_statusmsg, (void*)&text_toxid,
            (void*)&button_copyid,
            (void*)&edit_name, (void*)&edit_status,
            NULL
        }
    },

    {
        .type = PANEL_NONE,
        .disabled = 1,
        .content_scroll = &scroll_add,
        .drawfunc = drawadd,
        .child = (PANEL*[]) {
            //(void*)&text_name, (void*)&text_statusmsg, (void*)&text_toxid,
            (void*)&button_addfriend,
            (void*)&edit_addid, (void*)&edit_addmsg,
            NULL
        }
    },

    {
        .type = PANEL_NONE,
        .disabled = 1,
    },

    {
        .type = PANEL_NONE,
        .disabled = 1,
    },

    {
        .type = PANEL_NONE,
        .disabled = 1,
    },
},

panel_side = {
    .type = PANEL_NONE,
    .x = SIDE_X,
    .y = SIDE_Y,
    .width = -12 - SIDE_X,
    .height = -12,
    .child = (PANEL*[]) {
        &panel_item[0], &panel_item[1], NULL//&panel_friend, &panel_group, &panel_request, NULL
    }
},

panel_main = {
    .type = PANEL_MAIN,
    .x = 0,
    .y = 0,
    .width = 0,
    .height = 0,
    .child = (PANEL*[]) {
        &panel_list, &panel_side, (void*)&sysmenu, NULL
    }
};

#define FUNC(x, ret, ...) static ret (* x##func[])(void *p, ##__VA_ARGS__) = { \
    (void*)background_##x, \
    (void*)sysmenu_##x, \
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
#define FUNC() \
    x += (p->x < 0) ? width + p->x : p->x; \
    y += (p->y < 0) ? height + p->y : p->y; \
    width = (p->width <= 0) ? width + p->width : p->width; \
    height = (p->height <= 0) ? height + p->height : p->height; \

static void panel_draw_sub(PANEL *p, int x, int y, int width, int height)
{
    FUNC();

    //debug("test %u %i %i %i %i\n", p, x, y, width, height);

    if(p->type == PANEL_EDIT) {
        //debug("%i %i %i %i\n", x, y, width, height);
    }

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
        while(subp = *pp++) {
            if(!subp->disabled) {
                panel_draw_sub(subp, x, y, width, height);
            }
        }
    }

    popclip();
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
        while(subp = *pp++) {
            if(!subp->disabled) {
                panel_draw_sub(subp, x, y, width, height);
            }
        }
    }

    popclip();

    enddraw(x, y, width, height);
}

void panel_redraw(PANEL *p)
{
    /* just redraws everything for now */
    panel_draw(&panel_main, 0, 0, width, height);
}

void panel_mmove(PANEL *p, int x, int y, int width, int height, int mx, int my, int dy)
{
    mx -= (p->x < 0) ? width + p->x : p->x;
    my -= (p->y < 0) ? height + p->y : p->y;
    FUNC();

    _Bool draw = p->type ? mmovefunc[p->type - 1](p, mx, my, dy, width, height) : 0;
    PANEL **pp = p->child, *subp;
    if(pp) {
        while(subp = *pp++) {
            if(!subp->disabled) {
                panel_mmove(subp, x, y, width, height, mx, my, dy);
            }
        }
    }

    if(draw) {
        panel_redraw(NULL);
    }
}

void panel_mdown(PANEL *p)
{
    _Bool draw = p->type ? mdownfunc[p->type - 1](p) : 0;
    PANEL **pp = p->child, *subp;
    if(pp) {
        while(subp = *pp++) {
            if(!subp->disabled) {
                panel_mdown(subp);
            }
        }
    }

    if(draw) {
        panel_redraw(NULL);
    }
}

void panel_mright(PANEL *p)
{
    _Bool draw = p->type ? mrightfunc[p->type - 1](p) : 0;
    PANEL **pp = p->child, *subp;
    if(pp) {
        while(subp = *pp++) {
            if(!subp->disabled) {
                panel_mright(subp);
            }
        }
    }


    if(draw) {
        panel_redraw(NULL);
    }
}

void panel_mwheel(PANEL *p, int x, int y, int width, int height, double d)
{
    FUNC();

    _Bool draw = p->type ? mwheelfunc[p->type - 1](p, height, d) : 0;
    PANEL **pp = p->child, *subp;
    if(pp) {
        while(subp = *pp++) {
            if(!subp->disabled) {
                panel_mwheel(subp, x, y, width, height, d);
            }
        }
    }

    if(draw) {
        panel_redraw(NULL);
    }
}

void panel_mup(PANEL *p)
{
    _Bool draw = p->type ? mupfunc[p->type - 1](p) : 0;
    PANEL **pp = p->child, *subp;
    if(pp) {
        while(subp = *pp++) {
            if(!subp->disabled) {
                panel_mup(subp);
            }
        }
    }

    if(draw) {
        panel_redraw(NULL);
    }
}

void panel_mleave(PANEL *p)
{
    _Bool draw = p->type ? mleavefunc[p->type - 1](p) : 0;
    PANEL **pp = p->child, *subp;
    if(pp) {
        while(subp = *pp++) {
            if(!subp->disabled) {
                panel_mleave(subp);
            }
        }
    }

    if(draw) {
        panel_redraw(NULL);
    }
}

/*static void drawfriendmain(int x, int y, FRIEND *f)
{
    SetTextColor(hdc, 0x333333);
    SelectObject(hdc, font_big);
    drawtextrange(x + 8, width - 24, y + 2, f->name, f->name_length);

    SetTextColor(hdc, 0x999999);
    SelectObject(hdc, font_med);
    drawtextrange(x + 8, width - 24, y + 26, f->status_message, f->status_length);


    //RECT send = {width - 100 , height - 48, width - 24, height - 24};
    //FillRect(hdc, &send, red);

    draw_messages(x + 1, y + 47, f);

    //RECT r = {x, y + 46, width - 24, height - 152};

    if(f->online) {
        //FrameRect(hdc, &r, border);
    }

    switch(f->calling) {
    case 0: {
        button_call.text = "call";
        button_call.text_length = sizeof("call") - 1;
        break;
    }

    case 1: {
        button_call.text = "accept call";
        button_call.text_length = sizeof("accept call") - 1;
        break;
    }

    case 2: {
        button_call.text = "ringing..";
        button_call.text_length = sizeof("ringing..") - 1;
        break;
    }

    case 3: {
        button_call.text = "cancel call";
        button_call.text_length = sizeof("cancel call") - 1;
        break;
    }
    }

    button_draw(&button_call);

    edit_draw(&edit_msg);
}

static void drawfreqmain(int x, int y, FRIENDREQ *f)
{
    button_draw(&button_acceptfriend);
}

static void drawgroupmain(int x, int y, GROUPCHAT *g)
{
    draw_groupmessages(x + 1, y + 47, g);

    setcolor(0x333333);

    uint8_t **np = g->peername;
    int i = 0;
    while(i < g->peers) {
        uint8_t *n = *np++;
        if(n) {
            drawtextrange(width - 122, width - 24, y + 47 + i * 12, n + 1, n[0]);
            i++;
        }
    }

    edit_draw(&edit_msg);
}

static void drawaddmain(int x, int y)
{
    setcolor(0x333333);
    setfont(FONT_TITLE);

    drawstr(x, y + 2, "Add friends and groups");

    drawhline(x, y + 29, x + 300, INNER_BORDER);

    setcolor(0x555555);
    setfont(FONT_SUBTITLE);

    begindraw(x, y + 40, width - 12, height - 12);

    int dy = scroll_gety(&scroll_add);

    edit_addid.y = EDIT_ADDID_Y - dy;
    edit_addid.bottom = edit_addid.y + 24;

    edit_addmsg.y = EDIT_ADDMSG_Y - dy;
    edit_addmsg.bottom = edit_addmsg.y + 84;

    button_addfriend.y = MAIN_Y + 222 - dy;
    button_newgroup.y = MAIN_Y + 300 - dy;

    y -= dy;

    drawstr(x, y + 40, "Tox ID");
    drawstr(x, y + 90, "Message");

    setfont(FONT_TEXT_LARGE);

    if(addfriend_status) {
        drawtext(x, y + 200, addfriend_status_str[addfriend_status - 1], addfriend_status_length[addfriend_status - 1]);
    }

    setfont(FONT_SUBTITLE);
    drawstr(x, y + 280, "Create Group");

    edit_draw(&edit_addid);
    edit_draw(&edit_addmsg);

    button_draw(&button_addfriend);
    button_draw(&button_newgroup);

    scroll_draw(&scroll_add);

    enddraw();
}

void ui_drawmain(void)
{
    int x = MAIN_X, y = MAIN_Y;

    RECT area = {MAIN_X, MAIN_Y, width - 12, height - 12};
    fillrect(&area, COLOR_BG);

    SetBkMode(hdc, TRANSPARENT);

    switch(sitem->item) {
    case ITEM_FRIEND: {
        drawfriendmain(x, y, sitem->data);
        break;
    }

    case ITEM_GROUP: {
        drawgroupmain(x, y, sitem->data);
        break;
    }

    case ITEM_SELF: {
        drawselfmain(x, y);
        break;
    }

    case ITEM_ADDFRIEND: {
        drawaddmain(x, y);
        break;
    }

    case ITEM_FRIEND_ADD: {
        drawfreqmain(x, y, sitem->data);
        break;
    }
    }

    //scroll_func(scroll_draw);
    //edit_func(edit_draw);
    //button_func(button_draw);

    commitdraw(MAIN_X, MAIN_Y, width - 12 - MAIN_X, height - 12 - MAIN_Y);
}*/

/*void ui_updatesize(void)
{
    int x2 = (MAIN_X + 600) < (width - 24) ? MAIN_X + 600 : width - 24;

    edit_name.right = x2;
    edit_status.right = x2;

    edit_addid.right = x2;
    edit_addmsg.right = x2;

    button_addfriend.x = edit_addmsg.right - 50;

    edit_msg.y = height - 12 - 84 - 5;
    edit_msg.bottom = edit_msg.y + 84;
    edit_msg.right = width - 24;

    scroll_list.height = SCROLL_BOTTOM - LIST_Y;

    scroll_self.width = (width - 12) - (MAIN_X);
    scroll_add.width = (width - 12) - (MAIN_X);

    scroll_self.height = (height - 12) - (MAIN_Y + 40);
    scroll_add.height = (height - 12) - (MAIN_Y + 40);

    button_call.x = width - 12 - 50;
}*/

