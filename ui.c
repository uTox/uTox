#include "main.h"

#include "icons/contact.c"
#include "icons/group.c"

/* edits */
static uint8_t edit_name_data[128], edit_status_data[128], edit_addid_data[TOX_FRIEND_ADDRESS_SIZE * 2], edit_addmsg_data[1024], edit_msg_data[1024];

static void edit_name_onenter(void)
{
    uint8_t *data = edit_name_data;
    uint16_t length = edit_name.length;

    memcpy(self.name, data, length);
    self.name_length = length;

    tox_postmessage(TOX_SETNAME, length, 0, self.name);//!

    list_draw();
}

static void edit_status_onenter(void)
{
    uint8_t *data = edit_status_data;
    uint16_t length = edit_status.length;

    void *p = realloc(self.statusmsg, length);
    if(!p) {
        return;
    }

    self.statusmsg = p;
    memcpy(self.statusmsg, data, length);
    self.statusmsg_length = length;

    tox_postmessage(TOX_SETNAME, length, 0, self.statusmsg);//!

    list_draw();

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

    ui_drawmain();

    edit_clear();
    edit_draw(&edit_msg);

}

#define EDIT_NAME_Y (MAIN_Y + 64)
#define EDIT_STATUS_Y (MAIN_Y + 114)
#define EDIT_ADDID_Y (MAIN_Y + 64)
#define EDIT_ADDMSG_Y (MAIN_Y + 114)

EDIT edit_name = {
    .multiline = 0,
    .maxlength = 128,
    .x = MAIN_X,
    .y = EDIT_NAME_Y,
    .bottom = EDIT_NAME_Y + 24,
    .data = edit_name_data,
    .onenter = edit_name_onenter,
    .onredraw = ui_drawmain
},

edit_status = {
    .multiline = 0,
    .maxlength = 128,
    .x = MAIN_X,
    .y = EDIT_STATUS_Y,
    .bottom = EDIT_STATUS_Y + 24,
    .data = edit_status_data,
    .onenter = edit_status_onenter,
    .onredraw = ui_drawmain
},

edit_addid = {
    .multiline = 0,
    .maxlength = sizeof(edit_addid_data),
    .x = MAIN_X,
    .y = EDIT_ADDID_Y,
    .bottom = EDIT_ADDID_Y + 24,
    .data = edit_addid_data,
    .onredraw = ui_drawmain
},

edit_addmsg = {
    .multiline = 0,//1,
    .maxlength = sizeof(edit_addmsg_data),
    .x = MAIN_X,
    .y = EDIT_ADDMSG_Y,
    .bottom = EDIT_ADDMSG_Y + 84,
    .data = edit_addmsg_data,
    .onredraw = ui_drawmain
},

edit_msg = {
    .multiline = 0,//1,
    .maxlength = sizeof(edit_msg_data),
    .x = MAIN_X,
    .data = edit_msg_data,
    .onenter = edit_msg_onenter,
    .onredraw = ui_drawmain
};

/* buttons */

#define BUTTON_TEXT(x) .text = (uint8_t*)x, .text_length = sizeof(x) - 1

static void button_copyid_onpress(void)
{
    address_to_clipboard();
}

static void button_addfriend_onpress(void)
{
    uint8_t id[TOX_FRIEND_ADDRESS_SIZE];
    if(edit_addid.length != TOX_FRIEND_ADDRESS_SIZE * 2 || !string_to_id(id, edit_addid_data)) {
        addfriend_status = 2;
        ui_drawmain();
        return;
    }

    void *data = malloc(sizeof(id) + edit_addmsg.length);
    memcpy(data, id, sizeof(id));
    memcpy(data + sizeof(id), edit_addmsg.data, edit_addmsg.length);

    tox_postmessage(TOX_ADDFRIEND, edit_addmsg.length, 0, data);

    edit_setfocus(NULL);
}

static void button_newgroup_onpress(void)
{
    tox_postmessage(TOX_NEWGROUP, 0, 0, NULL);
}

static void button_call_onpress(void)
{
    FRIEND *f = sitem->data;

    switch(f->calling)
    {
        case 0:
        {
            tox_postmessage(TOX_CALL, f - friend, 0, NULL);
            debug("Calling friend: %u\n", f - friend);
            break;
        }

        case 1:
        {
            tox_postmessage(TOX_ACCEPTCALL, f->callid, 0, NULL);
            debug("Accept Call: %u\n", f->callid);
            break;
        }

        case 2:
        {
            //tox_postmessage(TOX_CALL, f - friend, 0, NULL);
            //debug("Calling friend: %u\n", f - friend);
            break;
        }

        case 3:
        {
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
    .x = MAIN_X,
    .y = MAIN_Y + 185,
    .width = 150,
    .height = 18,
    .onpress = button_copyid_onpress,
    .onredraw = ui_drawmain,
    BUTTON_TEXT("copy to clipboard")
},

button_addfriend = {
    .y = MAIN_Y + 222,
    .width = 50,
    .height = 18,
    .onpress = button_addfriend_onpress,
    .onredraw = ui_drawmain,
    BUTTON_TEXT("add")
},

button_newgroup = {
    .x = MAIN_X,
    .y = MAIN_Y + 300,
    .width = 50,
    .height = 18,
    .onpress = button_newgroup_onpress,
    .onredraw = ui_drawmain,
    BUTTON_TEXT("add")
},

button_call = {
    .y = MAIN_Y,
    .width = 50,
    .height = 18,
    .onredraw = ui_drawmain,
    .onpress = button_call_onpress,
    BUTTON_TEXT("call")
},

button_acceptfriend = {
    .x = MAIN_X,
    .y = MAIN_Y + 40,
    .width = 50,
    .height = 18,
    .onpress = button_acceptfriend_onpress,
    .onredraw = ui_drawmain,
    BUTTON_TEXT("add")
};


SCROLLABLE scroll_list = {
    .x = LIST_X,
    .y = LIST_Y,
    .width = ITEM_WIDTH,
    .onscroll = list_draw
},

scroll_self = {
    .x = MAIN_X,
    .y = MAIN_Y + 40,
    .content_height = 610 - (MAIN_Y + 40),
    .onscroll = ui_drawmain
},

scroll_add = {
    .x = MAIN_X,
    .y = MAIN_Y + 40,
    .content_height = 400,
    .onscroll = ui_drawmain
};

#define STRLEN(x) (sizeof(x) - 1)

static char *addfriend_status_str[] = {
    "Friend request sent",
    "Error: Invalid ID format",
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
    STRLEN("Error: Invalid ID format"),
    STRLEN("Error: Message too long"),
    STRLEN("Error: Empty message"),
    STRLEN("Error: ID is self"),
    STRLEN("Error: Friend request already sent"),
    STRLEN("Error: Unknown"),
    STRLEN("Error: Bad ID checksum"),
    STRLEN("Error: Bad ID nospam"),
    STRLEN("Error: No memory")
};

uint8_t
bm_minimize_bits[] = {
    0b00000000, 0b00000000,
    0b00000000, 0b00000000,
    0b00000000, 0b00000000,
    0b00000000, 0b00000000,
    0b00000000, 0b00000000,
    0b00000000, 0b00000000,
    0b00000000, 0b00000000,
    0b00000000, 0b00000000,
    0b00001111, 0b11110000,
    0b00001111, 0b11110000,
},

bm_maximize_bits[] = {
    0b00011111, 0b11111000,
    0b00011111, 0b11111000,
    0b00011111, 0b11111000,
    0b00010000, 0b00001000,
    0b00010000, 0b00001000,
    0b00010000, 0b00001000,
    0b00010000, 0b00001000,
    0b00010000, 0b00001000,
    0b00010000, 0b00001000,
    0b00011111, 0b11111000,
},

bm_restore_bits[] = {
    0b00000111, 0b11111000,
    0b00000111, 0b11111000,
    0b00000100, 0b00001000,
    0b00011111, 0b11101000,
    0b00011111, 0b11101000,
    0b00010000, 0b00101000,
    0b00010000, 0b00111000,
    0b00010000, 0b00100000,
    0b00010000, 0b00100000,
    0b00011111, 0b11100000,
},

bm_exit_bits[] = {
    0b00001000, 0b00010000,
    0b00011100, 0b00111000,
    0b00001110, 0b01110000,
    0b00000111, 0b11100000,
    0b00000011, 0b11000000,
    0b00000011, 0b11000000,
    0b00000111, 0b11100000,
    0b00001110, 0b01110000,
    0b00011100, 0b00111000,
    0b00001000, 0b00010000,
},

bm_plus_bits[] = {
    0b11111100, 0b00111111,
    0b11111100, 0b00111111,
    0b11111100, 0b00111111,
    0b11111100, 0b00111111,
    0b11111100, 0b00111111,
    0b11111100, 0b00111111,
    0, 0,
    0, 0,
    0, 0,
    0, 0,
    0b11111100, 0b00111111,
    0b11111100, 0b00111111,
    0b11111100, 0b00111111,
    0b11111100, 0b00111111,
    0b11111100, 0b00111111,
    0b11111100, 0b00111111,
};

#define F(r, g, b, a) (RGB((b * a) / 0xFF, (g * a) / 0xFF, (r * a) / 0xFF) | a << 24)
#define G(x) F(107, 194, 96, x)
uint32_t
bm_online_bits[] = {
    G(0  ), G(0  ), G(83 ), G(194), G(238), G(238), G(194), G(83 ), G(0  ), G(0  ),
    G(0  ), G(143), G(255), G(255), G(255), G(255), G(255), G(255), G(143), G(0  ),
    G(83 ), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(83 ),
    G(194), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(194),
    G(238), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(238),
    G(238), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(238),
    G(194), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(194),
    G(83 ), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(83 ),
    G(0  ), G(143), G(255), G(255), G(255), G(255), G(255), G(255), G(143), G(0  ),
    G(0  ), G(0  ), G(83 ), G(194), G(238), G(238), G(194), G(83 ), G(0  ), G(0  ),
},
#undef G
#define G(x) F(206, 191, 69, x)
bm_away_bits[] = {
    G(0  ), G(0  ), G(83 ), G(194), G(238), G(238), G(194), G(83 ), G(0  ), G(0  ),
    G(0  ), G(143), G(255), G(219), G(153), G(153), G(219), G(255), G(143), G(0  ),
    G(83 ), G(255), G(154), G(0  ), G(0  ), G(0  ), G(0  ), G(154), G(255), G(83 ),
    G(194), G(220), G(0  ), G(0  ), G(0  ), G(0  ), G(0  ), G(0  ), G(220), G(194),
    G(238), G(152), G(0  ), G(0  ), G(0  ), G(0  ), G(0  ), G(0  ), G(152), G(238),
    G(238), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(238),
    G(194), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(194),
    G(83 ), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(83 ),
    G(0  ), G(143), G(255), G(255), G(255), G(255), G(255), G(255), G(143), G(0  ),
    G(0  ), G(0  ), G(83 ), G(194), G(238), G(238), G(194), G(83 ), G(0  ), G(0  ),
},
#undef G
#define G(x) F(200, 78, 78, x)
bm_busy_bits[] = {
    G(0  ), G(0  ), G(83 ), G(194), G(238), G(238), G(194), G(83 ), G(0  ), G(0  ),
    G(0  ), G(143), G(255), G(255), G(255), G(255), G(255), G(255), G(143), G(0  ),
    G(83 ), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(83 ),
    G(194), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(194),
    G(238), G(128), G(0  ), G(0  ), G(0  ), G(0  ), G(0  ), G(0  ), G(128), G(238),
    G(238), G(128), G(0  ), G(0  ), G(0  ), G(0  ), G(0  ), G(0  ), G(128), G(238),
    G(194), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(194),
    G(83 ), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(255), G(83 ),
    G(0  ), G(143), G(255), G(255), G(255), G(255), G(255), G(255), G(143), G(0  ),
    G(0  ), G(0  ), G(83 ), G(194), G(238), G(238), G(194), G(83 ), G(0  ), G(0  ),
},
#undef G
#define G(x) F(200, 78, 78, x)
bm_offline_bits[] = {
    G(0  ), G(0  ), G(83 ), G(194), G(238), G(238), G(194), G(83 ), G(0  ), G(0  ),
    G(0  ), G(143), G(255), G(219), G(153), G(153), G(219), G(255), G(143), G(0  ),
    G(83 ), G(255), G(154), G(0  ), G(0  ), G(0  ), G(0  ), G(154), G(255), G(83 ),
    G(194), G(219), G(0  ), G(0  ), G(0  ), G(0  ), G(0  ), G(0  ), G(219), G(194),
    G(238), G(153), G(0  ), G(0  ), G(0  ), G(0  ), G(0  ), G(0  ), G(153), G(238),
    G(238), G(153), G(0  ), G(0  ), G(0  ), G(0  ), G(0  ), G(0  ), G(153), G(238),
    G(194), G(219), G(0  ), G(0  ), G(0  ), G(0  ), G(0  ), G(0  ), G(219), G(194),
    G(83 ), G(255), G(154), G(0  ), G(0  ), G(0  ), G(0  ), G(154), G(255), G(83 ),
    G(0  ), G(143), G(255), G(219), G(153), G(153), G(219), G(255), G(143), G(0  ),
    G(0  ), G(0  ), G(83 ), G(194), G(238), G(238), G(194), G(83 ), G(0  ), G(0  ),
};
#undef G

static void drawfriendmain(int x, int y, FRIEND *f)
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

    switch(f->calling)
    {
        case 0:
        {
            button_call.text = "call";
            button_call.text_length = sizeof("call") - 1;
            break;
        }

        case 1:
        {
            button_call.text = "accept call";
            button_call.text_length = sizeof("accept call") - 1;
            break;
        }

        case 2:
        {
            button_call.text = "ringing..";
            button_call.text_length = sizeof("ringing..") - 1;
            break;
        }

        case 3:
        {
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

static void drawselfmain(int x, int y)
{
    setcolor(0x333333);
    setfont(FONT_TITLE);

    drawstr(x, y + 2, "User settings");

    drawhline(x, y + 29, x + 200, INNER_BORDER);

    setcolor(0x555555);
    setfont(FONT_SUBTITLE);

    begindraw(x, y + 40, width - 12, height - 12);

    int dy = scroll_gety(&scroll_self);

    edit_name.y = EDIT_NAME_Y - dy;
    edit_name.bottom = edit_name.y + 24;

    edit_status.y = EDIT_STATUS_Y - dy;
    edit_status.bottom = edit_status.y + 24;

    button_copyid.y = MAIN_Y + 185 - dy;

    y -= dy;

    drawstr(x, y + 40, "Name");
    drawstr(x, y + 90, "Status message");
    drawstr(x, y + 140, "Tox ID");

    drawstr(x, y + 210, "Audio input device");
    drawstr(x, y + 260, "Audio output device");
    drawstr(x, y + 310, "Video input device");
    drawstr(x, y + 360, "Some other fun setting");
    drawstr(x, y + 410, "Most fun setting");
    drawstr(x, y + 460, "Even more fun setting");
    drawstr(x, y + 510, "Just enough so that it scrolls...");
    drawstr(x, y + 560, "on the default height");

    setfont(FONT_TEXT_LARGE);
    drawtextrange(x, width - 24, y + 165, self.id, sizeof(self.id));

    edit_draw(&edit_name);
    edit_draw(&edit_status);

    button_draw(&button_copyid);

    scroll_draw(&scroll_self);

    enddraw();
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

    case ITEM_FRIEND_ADD:
    {
        drawfreqmain(x, y, sitem->data);
        break;
    }
    }

    //scroll_func(scroll_draw);
    //edit_func(edit_draw);
    //button_func(button_draw);

    commitdraw(MAIN_X, MAIN_Y, width - 12 - MAIN_X, height - 12 - MAIN_Y);
}

void ui_drawbackground(void)
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

    commitdraw(0, 0, width, height);
}

void ui_updatesize(void)
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
}

