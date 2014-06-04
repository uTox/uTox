#include "main.h"

#include "icons/contact.c"
#include "icons/group.c"
#include "icons/misc.c"

#define STRLEN(x) (sizeof(x) - 1)

static char *addfriend_status_str[] = {
    "Friend request sent. Your friend will appear online when he accepts the request.",
    "Attempting to resolve DNS name...",
    "Error: Invalid Tox ID",
    "Error: No Tox ID specified",
    "Error: Message is too long",
    "Error: Empty message",
    "Error: Tox ID is self ID",
    "Error: Tox ID is already in friend list",
    "Error: Unknown",
    "Error: Invalid Tox ID (bad checksum)",
    "Error: Invalid Tox ID (bad nospam value)",
    "Error: No memory"
};

static uint16_t addfriend_status_length[] = {
    STRLEN("Friend request sent. Your friend will appear online when he accepts the request."),
    STRLEN("Attempting to resolve DNS name..."),
    STRLEN("Error: Invalid Tox ID"),
    STRLEN("Error: No Tox ID specified"),
    STRLEN("Error: Message is too long"),
    STRLEN("Error: Empty message"),
    STRLEN("Error: Tox ID is self ID"),
    STRLEN("Error: Tox ID is already in friend list"),
    STRLEN("Error: Unknown"),
    STRLEN("Error: Invalid Tox ID (bad checksum)"),
    STRLEN("Error: Invalid Tox ID (bad nospam value)"),
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

    redraw();
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

    redraw();
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
        .y = -94,
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

static void button_sendfile_onpress(void)
{
    char *filepath = malloc(1024);
    filepath[0] = 0;

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .hwndOwner = hwnd,
        .lpstrFile = filepath,
        .nMaxFile = 1024,
        .Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST,
    };

    if(GetOpenFileName(&ofn)) {
        tox_postmessage(TOX_SENDFILES, (FRIEND*)sitem->data - friend, ofn.nFileOffset, filepath);
    } else {
        debug("GetOpenFileName() failed\n");
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
        .x = -52,
        .y = 2,
        .width = 48,
        .height = 48,
    },
    .onpress = button_call_onpress,
               BUTTON_TEXT("call")
           },

button_sendfile = {
    .panel = {
        .type = PANEL_BUTTON,
        .x = -112,
        .y = 2,
        .width = 48,
        .height = 48,
    },
    .onpress = button_sendfile_onpress,
               BUTTON_TEXT("file")
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
};

static void drawself(int x, int y, int width, int height)
{
    setcolor(0x333333);
    setfont(FONT_TITLE);

    drawstr(x, y + 2, "User settings");

    setcolor(0x555555);
    setfont(FONT_SUBTITLE);

    drawstr(x, y + 40, "Name");
    drawstr(x, y + 90, "Status message");
    drawstr(x, y + 140, "Tox ID");

    drawhline(x, y + 29, x + 200, INNER_BORDER);

    setfont(FONT_TEXT_LARGE);
    drawtextwidth(x, width, y + 165, self.id, sizeof(self.id));
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
}

static void drawfriend(int x, int y, int width, int height)
{
    FRIEND *f = sitem->data;

    SetTextColor(hdc, 0x333333);
    setfont(FONT_TITLE);
    drawtextwidth(x, width, y + 2, f->name, f->name_length);

    SetTextColor(hdc, 0x999999);
    setfont(FONT_MED);
    drawtextwidth(x, width, y + 26, f->status_message, f->status_length);

    switch(f->calling) {
    case 0: {
        button_call.text = (uint8_t*)"call";
        button_call.text_length = sizeof("call") - 1;
        break;
    }

    case 1: {
        button_call.text = (uint8_t*)"accept call";
        button_call.text_length = sizeof("accept call") - 1;
        break;
    }

    case 2: {
        button_call.text = (uint8_t*)"ringing..";
        button_call.text_length = sizeof("ringing..") - 1;
        break;
    }

    case 3: {
        button_call.text = (uint8_t*)"cancel call";
        button_call.text_length = sizeof("cancel call") - 1;
        break;
    }
    }
}

static void drawgroup(int x, int y, int width, int height)
{

}

static void drawfriendreq(int x, int y, int width, int height)
{

}

static void drawfriendmsg(int x, int y, int width, int height)
{
    RECT r = {x, y, x + width, y + height};
    fillrect(&r, 0xDDDDDD);
}

static void drawgroupmsg(int x, int y, int width, int height)
{
    RECT r = {x, y, x + width, y + height};
    fillrect(&r, 0xDDDDDD);
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

panel_friendmessages = {
    .type = PANEL_NONE,
    .y = 50,
    .height = -94,
    .drawfunc = drawfriendmsg,
},

panel_groupmessages = {
    .y = 50,
    .height = -94,
    .drawfunc = drawgroupmsg,
},

panel_item[] = {
    {
        .type = PANEL_NONE,
        //.disabled = 1,
        .drawfunc = drawself,
        .child = (PANEL*[]) {
            (void*)&button_copyid,
            (void*)&edit_name, (void*)&edit_status,
            NULL
        }
    },

    {
        .type = PANEL_NONE,
        .disabled = 1,
        .drawfunc = drawadd,
        .child = (PANEL*[]) {
            (void*)&button_addfriend, (void*)&button_newgroup,
            (void*)&edit_addid, (void*)&edit_addmsg,
            NULL
        }
    },

    {
        .type = PANEL_NONE,
        .disabled = 1,
        .drawfunc = drawfriend,
        .child = (PANEL*[]) {
            (void*)&button_call, (void*)&button_sendfile,
            (void*)&edit_msg,
            (void*)&panel_friendmessages,
            NULL
        }
    },

    {
        .type = PANEL_NONE,
        .disabled = 1,
        .drawfunc = drawgroup,
        .child = (PANEL*[]) {
            (void*)&edit_msg,
            (void*)&panel_groupmessages,
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
    .x = SIDE_X,
    .y = SIDE_Y,
    .width = -12,
    .height = -12,
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

_Bool panel_mmove(PANEL *p, int x, int y, int width, int height, int mx, int my, int dy)
{
    mx -= (p->x < 0) ? width + p->x : p->x;
    my -= (p->y < 0) ? height + p->y : p->y;
    FUNC();

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

_Bool panel_mdown(PANEL *p)
{
    _Bool draw = p->type ? mdownfunc[p->type - 1](p) : 0;
    PANEL **pp = p->child, *subp;
    if(pp) {
        while((subp = *pp++)) {
            if(!subp->disabled) {
                draw |= panel_mdown(subp);
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
