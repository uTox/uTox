#include "main.h"
#include "theme_tables.h"

#define COLOR_PROC(a_ulColor) RGB((a_ulColor >> 16) & 0x0000FF, (a_ulColor >> 8) & 0x0000FF, a_ulColor & 0x0000FF)

/* Solarized color scheme */
#define SOLAR_BASE03 0x002b36
#define SOLAR_BASE02 0x073642
#define SOLAR_BASE01 0x586e75
#define SOLAR_BASE00 0x657b83
#define SOLAR_BASE0 0x839496
#define SOLAR_BASE1 0x93a1a1
#define SOLAR_BASE2 0xeee8d5
#define SOLAR_BASE3 0xfdf6e3
#define SOLAR_YELLOW 0xb58900
#define SOLAR_ORANGE 0xcb4b16
#define SOLAR_RED 0xdc322f
#define SOLAR_MAGENTA 0xd33682
#define SOLAR_VIOLET 0x6c71c4
#define SOLAR_BLUE 0x268bd2
#define SOLAR_CYAN 0x2aa198
#define SOLAR_GREEN 0x859900

void theme_load(char loadtheme) {
    // Update the settings dropdown UI

    // ==== Default theme     ====
    // ---- Background Colors ----
    COLOR_BKGRND_MAIN        = COLOR_PROC(0xffffff);
    COLOR_BKGRND_ALT         = COLOR_PROC(0xaaaaaa);
    COLOR_BKGRND_AUX         = COLOR_PROC(0x313131);
    COLOR_BKGRND_LIST        = COLOR_PROC(0x414141);
    COLOR_BKGRND_LIST_HOVER  = COLOR_PROC(0x505050);
    COLOR_BKGRND_MENU        = COLOR_PROC(0x1c1c1c);
    COLOR_BKGRND_MENU_HOVER  = COLOR_PROC(0x282828);
    COLOR_BKGRND_MENU_ACTIVE = COLOR_PROC(0x414141);

    /* ---- Text Colors --- */
    COLOR_MAIN_TEXT         = COLOR_PROC(0x333333);
    COLOR_MAIN_TEXT_CHAT    = COLOR_PROC(0x000000);
    COLOR_MAIN_TEXT_SUBTEXT = COLOR_PROC(0x414141);
    COLOR_MAIN_TEXT_ACTION  = COLOR_PROC(0x4e4ec8);
    COLOR_MAIN_TEXT_QUOTE   = COLOR_PROC(0x008000);
    COLOR_MAIN_TEXT_RED     = COLOR_PROC(0xFF0000);
    COLOR_MAIN_TEXT_URL     = COLOR_PROC(0x001fff);
    COLOR_MAIN_TEXT_HINT    = COLOR_PROC(0x969696);

    /* Message window colors */
    COLOR_MSG_USER       = COLOR_MAIN_TEXT_SUBTEXT;
    COLOR_MSG_USER_PEND  = COLOR_MAIN_TEXT_ACTION;
    COLOR_MSG_USER_ERROR = COLOR_MAIN_TEXT_RED;
    COLOR_MSG_CONTACT    = COLOR_MAIN_TEXT;

    //---- Friend list header and bottom-left buttons ----
    COLOR_MENU_TEXT         = COLOR_BKGRND_MAIN;
    COLOR_MENU_TEXT_SUBTEXT = COLOR_PROC(0xd1d1d1);
    COLOR_MENU_TEXT_ACTIVE  = COLOR_BKGRND_MAIN;

    //---- Friend list  ----
    COLOR_LIST_TEXT         = COLOR_MENU_TEXT;
    COLOR_LIST_TEXT_SUBTEXT = COLOR_MENU_TEXT_SUBTEXT;

    //---- Groupchat user list and title ----
    COLOR_GROUP_SELF  = COLOR_PROC(0x6bc260);
    COLOR_GROUP_PEER  = COLOR_MAIN_TEXT_HINT;
    COLOR_GROUP_AUDIO = COLOR_PROC(0xc84e4e);
    COLOR_GROUP_MUTED = COLOR_MAIN_TEXT_ACTION;

    //---- Text selection ----
    COLOR_SELECTION_BACKGROUND = COLOR_MAIN_TEXT;
    COLOR_SELECTION_TEXT       = COLOR_BKGRND_MAIN;

    //---- Inputs, dropdowns & tooltips ----
    COLOR_EDGE_NORMAL         = COLOR_PROC(0xc0c0c0);
    COLOR_EDGE_HOVER          = COLOR_PROC(0x969696);
    COLOR_EDGE_ACTIVE         = COLOR_PROC(0x4ea6ea);
    COLOR_ACTIVEOPTION_BKGRND = COLOR_PROC(0xd1d1d1);
    COLOR_ACTIVEOPTION_TEXT   = COLOR_MAIN_TEXT;

    //---- Auxiliary style for inputs/dropdowns ("Search friends" bar) ----
    COLOR_AUX_EDGE_NORMAL         = COLOR_BKGRND_AUX;
    COLOR_AUX_EDGE_HOVER          = COLOR_PROC(0x999999);
    COLOR_AUX_EDGE_ACTIVE         = COLOR_PROC(0x1A73B7);
    COLOR_AUX_TEXT                = COLOR_LIST_TEXT;
    COLOR_AUX_ACTIVEOPTION_BKGRND = COLOR_BKGRND_LIST_HOVER;
    COLOR_AUX_ACTIVEOPTION_TEXT   = COLOR_AUX_TEXT;

    //---- Status circles ----
    COLOR_STATUS_ONLINE = COLOR_PROC(0x6bc260);
    COLOR_STATUS_AWAY   = COLOR_PROC(0xcebf45);
    COLOR_STATUS_BUSY   = COLOR_PROC(0xc84e4e);

    //---- Buttons ----
    COLOR_BTN_SUCCESS_BKGRND       = COLOR_STATUS_ONLINE;
    COLOR_BTN_SUCCESS_BKGRND_HOVER = COLOR_PROC(0x76d56a);
    COLOR_BTN_SUCCESS_TEXT         = COLOR_BKGRND_MAIN;
    COLOR_BTN_SUCCESS_TEXT_HOVER   = COLOR_BKGRND_MAIN;

    COLOR_BTN_WARNING_BKGRND       = COLOR_STATUS_AWAY;
    COLOR_BTN_WARNING_BKGRND_HOVER = COLOR_PROC(0xe3d24c);
    COLOR_BTN_WARNING_TEXT         = COLOR_BKGRND_MAIN;
    COLOR_BTN_WARNING_TEXT_HOVER   = COLOR_BKGRND_MAIN;

    COLOR_BTN_DANGER_BACKGROUND   = COLOR_STATUS_BUSY;
    COLOR_BTN_DANGER_BKGRND_HOVER = COLOR_PROC(0xdc5656);
    COLOR_BTN_DANGER_TEXT         = COLOR_BKGRND_MAIN;
    COLOR_BTN_DANGER_TEXT_HOVER   = COLOR_BKGRND_MAIN;

    COLOR_BTN_DISABLED_BKGRND       = COLOR_PROC(0xd1d1d1);
    COLOR_BTN_DISABLED_BKGRND_HOVER = COLOR_BKGRND_LIST_HOVER;
    COLOR_BTN_DISABLED_TEXT         = COLOR_BKGRND_MAIN;
    COLOR_BTN_DISABLED_TRANSFER     = COLOR_BKGRND_LIST;
    COLOR_BTN_DISABLED_FORGRND      = COLOR_PROC(0xb3b3b3);

    COLOR_BTN_INPROGRESS_BKGRND  = COLOR_PROC(0x4ea6ea);
    COLOR_BTN_INPROGRESS_TEXT    = COLOR_BKGRND_MAIN;
    COLOR_BTN_INPROGRESS_FORGRND = COLOR_PROC(0x76baef);

    switch (loadtheme) {
        case THEME_DARK:
            COLOR_BKGRND_MAIN        = COLOR_PROC(0x333333);
            COLOR_BKGRND_ALT         = COLOR_PROC(0x151515);
            COLOR_BKGRND_AUX         = COLOR_BKGRND_MENU;
            COLOR_BKGRND_LIST        = COLOR_PROC(0x222222);
            COLOR_BKGRND_LIST_HOVER  = COLOR_PROC(0x151515);
            COLOR_BKGRND_MENU        = COLOR_PROC(0x171717);
            COLOR_BKGRND_MENU_HOVER  = COLOR_BKGRND_LIST_HOVER;
            COLOR_BKGRND_MENU_ACTIVE = COLOR_BKGRND_LIST;

            COLOR_MAIN_TEXT         = COLOR_PROC(0xdfdfdf);
            COLOR_MAIN_TEXT_CHAT    = COLOR_PROC(0xffffff);
            COLOR_MAIN_TEXT_SUBTEXT = COLOR_PROC(0xbbbbbb);
            COLOR_MAIN_TEXT_ACTION  = COLOR_PROC(0x27a9bc);
            COLOR_MAIN_TEXT_URL     = COLOR_MAIN_TEXT_ACTION;
            COLOR_MAIN_TEXT_QUOTE   = COLOR_PROC(0x55b317);

            COLOR_MSG_USER       = COLOR_MAIN_TEXT_SUBTEXT;
            COLOR_MSG_USER_PEND  = COLOR_PROC(0x66ccff);
            COLOR_MSG_USER_ERROR = COLOR_MAIN_TEXT_RED;
            COLOR_MSG_CONTACT    = COLOR_MAIN_TEXT;

            COLOR_SELECTION_BACKGROUND = COLOR_MAIN_TEXT;
            COLOR_SELECTION_TEXT       = COLOR_BKGRND_MAIN;
            COLOR_GROUP_MUTED          = COLOR_MAIN_TEXT_URL;
            COLOR_EDGE_NORMAL          = COLOR_PROC(0x555555);
            COLOR_EDGE_ACTIVE          = COLOR_PROC(0x228888);
            COLOR_EDGE_HOVER           = COLOR_PROC(0x999999);

            COLOR_ACTIVEOPTION_BKGRND     = COLOR_PROC(0x228888);
            COLOR_ACTIVEOPTION_TEXT       = COLOR_MAIN_TEXT;
            COLOR_AUX_EDGE_NORMAL         = COLOR_BKGRND_AUX;
            COLOR_AUX_EDGE_ACTIVE         = COLOR_EDGE_ACTIVE;
            COLOR_AUX_ACTIVEOPTION_BKGRND = COLOR_ACTIVEOPTION_BKGRND;
            COLOR_MENU_TEXT_ACTIVE        = COLOR_MAIN_TEXT;

            COLOR_BTN_SUCCESS_BKGRND       = COLOR_PROC(0x414141);
            COLOR_BTN_SUCCESS_TEXT         = COLOR_PROC(0x33a63d);
            COLOR_BTN_SUCCESS_BKGRND_HOVER = COLOR_PROC(0x455147);
            COLOR_BTN_SUCCESS_TEXT_HOVER   = COLOR_PROC(0x6eff3a);
            COLOR_BTN_WARNING_BKGRND       = COLOR_PROC(0x414141);
            COLOR_BTN_WARNING_TEXT         = COLOR_PROC(0xbd9e22);
            COLOR_BTN_WARNING_BKGRND_HOVER = COLOR_PROC(0x4c493c);
            COLOR_BTN_WARNING_TEXT_HOVER   = COLOR_PROC(0xff8d2a);
            COLOR_BTN_DANGER_BACKGROUND    = COLOR_PROC(0x414141);
            COLOR_BTN_DANGER_TEXT          = COLOR_PROC(0xbd2525);
            COLOR_BTN_DANGER_BKGRND_HOVER  = COLOR_PROC(0x513939);
            COLOR_BTN_DANGER_TEXT_HOVER    = COLOR_PROC(0xfa2626);
            COLOR_BTN_DISABLED_BKGRND      = COLOR_PROC(0x414141);
            COLOR_BTN_DISABLED_TEXT        = COLOR_MAIN_TEXT;
            COLOR_BTN_DISABLED_TRANSFER    = COLOR_BTN_DISABLED_TEXT;
            COLOR_BTN_INPROGRESS_BKGRND    = COLOR_BTN_DISABLED_BKGRND;
            COLOR_BTN_INPROGRESS_TEXT      = COLOR_MAIN_TEXT_URL;
            COLOR_BTN_DISABLED_FORGRND     = COLOR_PROC(0x666666);
            COLOR_BTN_INPROGRESS_FORGRND   = COLOR_PROC(0x2f656a);
            break;

        case THEME_LIGHT:
            COLOR_BKGRND_LIST             = COLOR_PROC(0xf0f0f0);
            COLOR_BKGRND_LIST_HOVER       = COLOR_PROC(0xe0e0e0);
            COLOR_BKGRND_MENU             = COLOR_BKGRND_LIST;
            COLOR_BKGRND_MENU_HOVER       = COLOR_PROC(0xe0e0e0);
            COLOR_BKGRND_MENU_ACTIVE      = COLOR_PROC(0x555555);
            COLOR_LIST_TEXT               = COLOR_MAIN_TEXT;
            COLOR_LIST_TEXT_SUBTEXT       = COLOR_MAIN_TEXT_SUBTEXT;
            COLOR_MENU_TEXT               = COLOR_PROC(0x555555);
            COLOR_MENU_TEXT_ACTIVE        = COLOR_PROC(0xffffff);
            COLOR_MENU_TEXT_SUBTEXT       = COLOR_PROC(0x414141);
            COLOR_EDGE_NORMAL             = COLOR_PROC(0xc0c0c0);
            COLOR_EDGE_HOVER              = COLOR_PROC(0x707070);
            COLOR_ACTIVEOPTION_BKGRND     = COLOR_PROC(0xc2e0ff);
            COLOR_ACTIVEOPTION_TEXT       = COLOR_MAIN_TEXT;
            COLOR_BKGRND_AUX              = COLOR_PROC(0xe0e0e0);
            COLOR_AUX_EDGE_NORMAL         = COLOR_BKGRND_AUX;
            COLOR_AUX_EDGE_HOVER          = COLOR_PROC(0x999999);
            COLOR_AUX_EDGE_ACTIVE         = COLOR_EDGE_ACTIVE;
            COLOR_AUX_TEXT                = COLOR_LIST_TEXT;
            COLOR_AUX_ACTIVEOPTION_BKGRND = COLOR_ACTIVEOPTION_BKGRND;
            COLOR_AUX_ACTIVEOPTION_TEXT   = COLOR_AUX_TEXT;
            break;

        case THEME_HIGHCONTRAST:
            COLOR_BKGRND_MAIN              = COLOR_PROC(0xffffff);
            COLOR_BKGRND_AUX               = COLOR_BKGRND_MAIN;
            COLOR_BKGRND_LIST              = COLOR_PROC(0x444444);
            COLOR_BKGRND_LIST_HOVER        = COLOR_MAIN_TEXT;
            COLOR_BKGRND_MENU              = COLOR_BKGRND_MAIN;
            COLOR_BKGRND_MENU_HOVER        = COLOR_BKGRND_MAIN;
            COLOR_BKGRND_MENU_ACTIVE       = COLOR_MAIN_TEXT;
            COLOR_MAIN_TEXT                = COLOR_PROC(0x000001);
            COLOR_MAIN_TEXT_CHAT           = COLOR_MAIN_TEXT;
            COLOR_MAIN_TEXT_SUBTEXT        = COLOR_MAIN_TEXT;
            COLOR_MAIN_TEXT_ACTION         = COLOR_PROC(0x0000ff);
            COLOR_MAIN_TEXT_QUOTE          = COLOR_PROC(0x00ff00);
            COLOR_MAIN_TEXT_URL            = COLOR_MAIN_TEXT_ACTION;
            COLOR_MAIN_TEXT_HINT           = COLOR_MAIN_TEXT;
            COLOR_MENU_TEXT                = COLOR_MAIN_TEXT;
            COLOR_MENU_TEXT_SUBTEXT        = COLOR_MAIN_TEXT;
            COLOR_MENU_TEXT_ACTIVE         = COLOR_BKGRND_MAIN;
            COLOR_LIST_TEXT                = COLOR_BKGRND_MAIN;
            COLOR_LIST_TEXT_SUBTEXT        = COLOR_BKGRND_MAIN;
            COLOR_GROUP_SELF               = COLOR_PROC(0x00ff00);
            COLOR_GROUP_PEER               = COLOR_MAIN_TEXT_HINT;
            COLOR_GROUP_AUDIO              = COLOR_PROC(0xff0000);
            COLOR_GROUP_MUTED              = COLOR_MAIN_TEXT_URL;
            COLOR_SELECTION_BACKGROUND     = COLOR_MAIN_TEXT;
            COLOR_SELECTION_TEXT           = COLOR_BKGRND_MAIN;
            COLOR_EDGE_NORMAL              = COLOR_MAIN_TEXT;
            COLOR_EDGE_ACTIVE              = COLOR_MAIN_TEXT;
            COLOR_EDGE_HOVER               = COLOR_MAIN_TEXT;
            COLOR_ACTIVEOPTION_BKGRND      = COLOR_MAIN_TEXT;
            COLOR_ACTIVEOPTION_TEXT        = COLOR_BKGRND_MAIN;
            COLOR_STATUS_ONLINE            = COLOR_PROC(0x00ff00);
            COLOR_STATUS_AWAY              = COLOR_PROC(0xffff00);
            COLOR_STATUS_BUSY              = COLOR_PROC(0xff0000);
            COLOR_BTN_SUCCESS_BKGRND       = COLOR_PROC(0x00ff00);
            COLOR_BTN_SUCCESS_TEXT         = COLOR_BKGRND_MAIN;
            COLOR_BTN_SUCCESS_BKGRND_HOVER = COLOR_PROC(0x00ff00);
            COLOR_BTN_SUCCESS_TEXT_HOVER   = COLOR_BKGRND_MAIN;
            COLOR_BTN_WARNING_BKGRND       = COLOR_PROC(0xffff00);
            COLOR_BTN_WARNING_TEXT         = COLOR_BKGRND_MAIN;
            COLOR_BTN_WARNING_BKGRND_HOVER = COLOR_PROC(0xffff00);
            COLOR_BTN_WARNING_TEXT_HOVER   = COLOR_BKGRND_MAIN;
            COLOR_BTN_DANGER_BACKGROUND    = COLOR_PROC(0xff0000);
            COLOR_BTN_DANGER_TEXT          = COLOR_BKGRND_MAIN;
            COLOR_BTN_DANGER_BKGRND_HOVER  = COLOR_PROC(0xff0000);
            COLOR_BTN_DANGER_TEXT_HOVER    = COLOR_BKGRND_MAIN;
            COLOR_BTN_DISABLED_BKGRND      = COLOR_PROC(0x444444);
            COLOR_BTN_DISABLED_TEXT        = COLOR_MAIN_TEXT;
            COLOR_BTN_DISABLED_TRANSFER    = COLOR_BKGRND_MAIN;
            COLOR_BTN_INPROGRESS_TEXT      = COLOR_BTN_DISABLED_TEXT;
            COLOR_BTN_INPROGRESS_BKGRND    = COLOR_PROC(0x00ffff);
            COLOR_AUX_EDGE_NORMAL          = COLOR_EDGE_NORMAL;
            COLOR_AUX_EDGE_HOVER           = COLOR_EDGE_NORMAL;
            COLOR_AUX_EDGE_ACTIVE          = COLOR_EDGE_ACTIVE;
            COLOR_AUX_TEXT                 = COLOR_MAIN_TEXT;
            COLOR_AUX_ACTIVEOPTION_BKGRND  = COLOR_ACTIVEOPTION_BKGRND;
            COLOR_AUX_ACTIVEOPTION_TEXT    = COLOR_ACTIVEOPTION_TEXT;
            COLOR_BTN_DISABLED_FORGRND     = COLOR_PROC(0x000000);
            break;

        case THEME_ZENBURN:
            COLOR_BKGRND_MAIN               = COLOR_PROC(0x3f3f3f);
            COLOR_BKGRND_AUX                = COLOR_BKGRND_MAIN;
            COLOR_BKGRND_LIST               = COLOR_PROC(0x5f5f5f);
            COLOR_BKGRND_LIST_HOVER         = COLOR_PROC(0x7f7f7f);
            COLOR_BKGRND_MENU               = COLOR_BKGRND_MAIN;
            COLOR_BKGRND_MENU_HOVER         = COLOR_MAIN_TEXT_QUOTE;
            COLOR_BKGRND_MENU_ACTIVE        = COLOR_MAIN_TEXT_QUOTE;
            COLOR_MAIN_TEXT                 = COLOR_PROC(0xdcdccc);
            COLOR_MAIN_TEXT_CHAT            = COLOR_MAIN_TEXT;
            COLOR_MAIN_TEXT_SUBTEXT         = COLOR_MAIN_TEXT;
            COLOR_MAIN_TEXT_ACTION          = COLOR_PROC(0xd0bf8f);
            COLOR_MAIN_TEXT_QUOTE           = COLOR_PROC(0x7f9f7f);
            COLOR_MAIN_TEXT_URL             = COLOR_PROC(0x6ca0a3);
            COLOR_MAIN_TEXT_HINT            = COLOR_MAIN_TEXT;
            COLOR_MENU_TEXT                 = COLOR_MAIN_TEXT;
            COLOR_MENU_TEXT_SUBTEXT         = COLOR_MAIN_TEXT;
            COLOR_MENU_TEXT_ACTIVE          = COLOR_MAIN_TEXT;
            COLOR_LIST_TEXT                 = COLOR_MAIN_TEXT;
            COLOR_LIST_TEXT_SUBTEXT         = COLOR_MAIN_TEXT;
            COLOR_AUX_EDGE_NORMAL           = COLOR_BKGRND_LIST;
            COLOR_AUX_EDGE_HOVER            = COLOR_MAIN_TEXT_QUOTE;
            COLOR_AUX_EDGE_ACTIVE           = COLOR_MAIN_TEXT;
            COLOR_AUX_TEXT                  = COLOR_MAIN_TEXT;
            COLOR_AUX_ACTIVEOPTION_BKGRND   = COLOR_MAIN_TEXT_QUOTE;
            COLOR_AUX_ACTIVEOPTION_TEXT     = COLOR_MAIN_TEXT;
            COLOR_GROUP_SELF                = COLOR_MAIN_TEXT;
            COLOR_GROUP_PEER                = COLOR_MAIN_TEXT;
            COLOR_GROUP_AUDIO               = COLOR_MAIN_TEXT_QUOTE;
            COLOR_GROUP_MUTED               = COLOR_MAIN_TEXT_ACTION;
            COLOR_SELECTION_BACKGROUND      = COLOR_MAIN_TEXT_QUOTE;
            COLOR_SELECTION_TEXT            = COLOR_MAIN_TEXT;
            COLOR_EDGE_NORMAL               = COLOR_BKGRND_LIST;
            COLOR_EDGE_ACTIVE               = COLOR_MAIN_TEXT;
            COLOR_EDGE_HOVER                = COLOR_MAIN_TEXT_QUOTE;
            COLOR_ACTIVEOPTION_BKGRND       = COLOR_MAIN_TEXT_QUOTE;
            COLOR_ACTIVEOPTION_TEXT         = COLOR_MAIN_TEXT;
            COLOR_STATUS_ONLINE             = COLOR_MAIN_TEXT_QUOTE;
            COLOR_STATUS_AWAY               = COLOR_MAIN_TEXT_ACTION;
            COLOR_STATUS_BUSY               = COLOR_PROC(0xcc9393);
            COLOR_BTN_SUCCESS_BKGRND        = COLOR_MAIN_TEXT_QUOTE;
            COLOR_BTN_SUCCESS_TEXT          = COLOR_MAIN_TEXT;
            COLOR_BTN_SUCCESS_BKGRND_HOVER  = COLOR_PROC(0xbfebbf);
            COLOR_BTN_SUCCESS_TEXT_HOVER    = COLOR_PROC(0xffffff);
            COLOR_BTN_WARNING_BKGRND        = COLOR_MAIN_TEXT_ACTION;
            COLOR_BTN_WARNING_TEXT          = COLOR_BTN_SUCCESS_TEXT_HOVER;
            COLOR_BTN_WARNING_BKGRND_HOVER  = COLOR_PROC(0xf0dfaf);
            COLOR_BTN_WARNING_TEXT_HOVER    = COLOR_BTN_SUCCESS_TEXT_HOVER;
            COLOR_BTN_DANGER_BACKGROUND     = COLOR_STATUS_AWAY;
            COLOR_BTN_DANGER_TEXT           = COLOR_MAIN_TEXT;
            COLOR_BTN_DANGER_BKGRND_HOVER   = COLOR_PROC(0xdca3a3);
            COLOR_BTN_DANGER_TEXT_HOVER     = COLOR_BTN_SUCCESS_TEXT_HOVER;
            COLOR_BTN_DISABLED_BKGRND       = COLOR_BKGRND_LIST;
            COLOR_BTN_DISABLED_TEXT         = COLOR_MAIN_TEXT;
            COLOR_BTN_DISABLED_BKGRND_HOVER = COLOR_BKGRND_LIST_HOVER;
            COLOR_BTN_DISABLED_TRANSFER     = COLOR_MAIN_TEXT;
            COLOR_BTN_INPROGRESS_BKGRND     = COLOR_PROC(0xc1c1a4);
            COLOR_BTN_INPROGRESS_TEXT       = COLOR_BKGRND_MAIN;
            COLOR_BTN_INPROGRESS_FORGRND    = COLOR_MAIN_TEXT;
            COLOR_BTN_DISABLED_FORGRND      = COLOR_BKGRND_LIST_HOVER;
            break;

        case THEME_SOLARIZED_DARK:
            COLOR_BKGRND_MAIN        = COLOR_PROC(SOLAR_BASE03);
            COLOR_BKGRND_ALT         = COLOR_PROC(SOLAR_BASE02);
            COLOR_BKGRND_AUX         = COLOR_BKGRND_ALT;
            COLOR_BKGRND_LIST        = COLOR_BKGRND_ALT;
            COLOR_BKGRND_LIST_HOVER  = COLOR_PROC(SOLAR_BASE01);
            COLOR_BKGRND_MENU        = COLOR_PROC(SOLAR_BASE03);
            COLOR_BKGRND_MENU_HOVER  = COLOR_PROC(SOLAR_CYAN);
            COLOR_BKGRND_MENU_ACTIVE = COLOR_BKGRND_ALT;

            COLOR_MAIN_TEXT         = COLOR_PROC(SOLAR_BASE2);
            COLOR_MAIN_TEXT_CHAT    = COLOR_MAIN_TEXT;
            COLOR_MAIN_TEXT_SUBTEXT = COLOR_PROC(SOLAR_BASE1);
            COLOR_MAIN_TEXT_ACTION  = COLOR_PROC(SOLAR_BASE3);
            COLOR_MAIN_TEXT_QUOTE   = COLOR_MAIN_TEXT_SUBTEXT;
            COLOR_MAIN_TEXT_RED     = COLOR_PROC(SOLAR_RED);
            COLOR_MAIN_TEXT_URL     = COLOR_PROC(SOLAR_MAGENTA);
            COLOR_MAIN_TEXT_HINT    = COLOR_PROC(SOLAR_VIOLET);

            COLOR_MSG_USER       = COLOR_MAIN_TEXT_SUBTEXT;
            COLOR_MSG_USER_PEND  = COLOR_MAIN_TEXT_ACTION;
            COLOR_MSG_USER_ERROR = COLOR_MAIN_TEXT_RED;
            COLOR_MSG_CONTACT    = COLOR_MAIN_TEXT;

            COLOR_MENU_TEXT         = COLOR_MAIN_TEXT;
            COLOR_MENU_TEXT_SUBTEXT = COLOR_MAIN_TEXT_SUBTEXT;
            COLOR_MENU_TEXT_ACTIVE  = COLOR_MAIN_TEXT;

            COLOR_LIST_TEXT         = COLOR_MAIN_TEXT;
            COLOR_LIST_TEXT_SUBTEXT = COLOR_MAIN_TEXT_SUBTEXT;

            COLOR_GROUP_SELF  = COLOR_PROC(SOLAR_GREEN);
            COLOR_GROUP_PEER  = COLOR_MAIN_TEXT_HINT;
            COLOR_GROUP_AUDIO = COLOR_PROC(SOLAR_RED);
            COLOR_GROUP_MUTED = COLOR_MAIN_TEXT_ACTION;

            COLOR_SELECTION_BACKGROUND = COLOR_MAIN_TEXT;
            COLOR_SELECTION_TEXT       = COLOR_BKGRND_MAIN;

            COLOR_EDGE_NORMAL         = COLOR_PROC(SOLAR_VIOLET);
            COLOR_EDGE_HOVER          = COLOR_PROC(SOLAR_BLUE);
            COLOR_EDGE_ACTIVE         = COLOR_PROC(SOLAR_ORANGE);
            COLOR_EDGE_ACTIVE         = COLOR_PROC(SOLAR_CYAN);
            COLOR_ACTIVEOPTION_BKGRND = COLOR_BKGRND_LIST_HOVER;
            COLOR_ACTIVEOPTION_TEXT   = COLOR_MAIN_TEXT;

            COLOR_AUX_EDGE_NORMAL         = COLOR_BKGRND_AUX;
            COLOR_AUX_EDGE_HOVER          = COLOR_PROC(SOLAR_VIOLET);
            COLOR_AUX_EDGE_ACTIVE         = COLOR_PROC(SOLAR_CYAN);
            COLOR_AUX_TEXT                = COLOR_LIST_TEXT;
            COLOR_AUX_ACTIVEOPTION_BKGRND = COLOR_BKGRND_LIST_HOVER;
            COLOR_AUX_ACTIVEOPTION_TEXT   = COLOR_AUX_TEXT;

            COLOR_STATUS_ONLINE = COLOR_PROC(SOLAR_GREEN);
            COLOR_STATUS_AWAY   = COLOR_PROC(SOLAR_YELLOW);
            COLOR_STATUS_BUSY   = COLOR_PROC(SOLAR_RED);

            COLOR_BTN_SUCCESS_BKGRND        = COLOR_STATUS_ONLINE;
            COLOR_BTN_SUCCESS_TEXT          = COLOR_MAIN_TEXT;
            COLOR_BTN_SUCCESS_BKGRND_HOVER  = COLOR_PROC(SOLAR_CYAN);
            COLOR_BTN_SUCCESS_TEXT_HOVER    = COLOR_BKGRND_MAIN;
            COLOR_BTN_WARNING_BKGRND        = COLOR_STATUS_AWAY;
            COLOR_BTN_WARNING_TEXT          = COLOR_MAIN_TEXT;
            COLOR_BTN_WARNING_BKGRND_HOVER  = COLOR_PROC(SOLAR_ORANGE);
            COLOR_BTN_WARNING_TEXT_HOVER    = COLOR_BKGRND_MAIN;
            COLOR_BTN_DANGER_BACKGROUND     = COLOR_STATUS_BUSY;
            COLOR_BTN_DANGER_TEXT           = COLOR_MAIN_TEXT;
            COLOR_BTN_DANGER_BKGRND_HOVER   = COLOR_PROC(SOLAR_MAGENTA);
            COLOR_BTN_DANGER_TEXT_HOVER     = COLOR_BKGRND_MAIN;
            COLOR_BTN_DISABLED_BKGRND       = COLOR_PROC(SOLAR_BASE00);
            COLOR_BTN_DISABLED_TEXT         = COLOR_BKGRND_MAIN;
            COLOR_BTN_DISABLED_BKGRND_HOVER = COLOR_BKGRND_LIST_HOVER;
            COLOR_BTN_DISABLED_TRANSFER     = COLOR_BKGRND_LIST;
            COLOR_BTN_INPROGRESS_BKGRND     = COLOR_PROC(SOLAR_VIOLET);
            COLOR_BTN_INPROGRESS_TEXT       = COLOR_BKGRND_MAIN;

            COLOR_BTN_DISABLED_FORGRND   = COLOR_PROC(SOLAR_ORANGE);
            COLOR_BTN_INPROGRESS_FORGRND = COLOR_PROC(SOLAR_MAGENTA);
            break;

        case THEME_SOLARIZED_LIGHT:
            COLOR_BKGRND_MAIN        = COLOR_PROC(SOLAR_BASE3);
            COLOR_BKGRND_ALT         = COLOR_PROC(SOLAR_BASE2);
            COLOR_BKGRND_AUX         = COLOR_BKGRND_ALT;
            COLOR_BKGRND_LIST        = COLOR_BKGRND_ALT;
            COLOR_BKGRND_LIST_HOVER  = COLOR_PROC(SOLAR_BASE1);
            COLOR_BKGRND_MENU        = COLOR_BKGRND_ALT;
            COLOR_BKGRND_MENU_HOVER  = COLOR_PROC(SOLAR_CYAN);
            COLOR_BKGRND_MENU_ACTIVE = COLOR_BKGRND_ALT;

            COLOR_MAIN_TEXT         = COLOR_PROC(SOLAR_BASE02);
            COLOR_MAIN_TEXT_CHAT    = COLOR_MAIN_TEXT;
            COLOR_MAIN_TEXT_SUBTEXT = COLOR_PROC(SOLAR_BASE01);
            COLOR_MAIN_TEXT_ACTION  = COLOR_PROC(SOLAR_BASE03);
            COLOR_MAIN_TEXT_QUOTE   = COLOR_MAIN_TEXT_SUBTEXT;
            COLOR_MAIN_TEXT_RED     = COLOR_PROC(SOLAR_RED);
            COLOR_MAIN_TEXT_URL     = COLOR_PROC(SOLAR_MAGENTA);
            COLOR_MAIN_TEXT_HINT    = COLOR_PROC(SOLAR_VIOLET);

            COLOR_MSG_USER       = COLOR_MAIN_TEXT_SUBTEXT;
            COLOR_MSG_USER_PEND  = COLOR_MAIN_TEXT_ACTION;
            COLOR_MSG_USER_ERROR = COLOR_MAIN_TEXT_RED;
            COLOR_MSG_CONTACT    = COLOR_MAIN_TEXT;

            COLOR_MENU_TEXT         = COLOR_MAIN_TEXT;
            COLOR_MENU_TEXT_SUBTEXT = COLOR_MAIN_TEXT_SUBTEXT;
            COLOR_MENU_TEXT_ACTIVE  = COLOR_MAIN_TEXT;

            COLOR_LIST_TEXT         = COLOR_MAIN_TEXT;
            COLOR_LIST_TEXT_SUBTEXT = COLOR_MAIN_TEXT_SUBTEXT;

            COLOR_GROUP_SELF  = COLOR_PROC(SOLAR_GREEN);
            COLOR_GROUP_PEER  = COLOR_MAIN_TEXT_HINT;
            COLOR_GROUP_AUDIO = COLOR_PROC(SOLAR_RED);
            COLOR_GROUP_MUTED = COLOR_MAIN_TEXT_ACTION;

            COLOR_SELECTION_BACKGROUND = COLOR_MAIN_TEXT;
            COLOR_SELECTION_TEXT       = COLOR_BKGRND_MAIN;

            COLOR_EDGE_NORMAL         = COLOR_PROC(SOLAR_VIOLET);
            COLOR_EDGE_HOVER          = COLOR_PROC(SOLAR_BLUE);
            COLOR_EDGE_ACTIVE         = COLOR_PROC(SOLAR_ORANGE);
            COLOR_EDGE_ACTIVE         = COLOR_PROC(SOLAR_CYAN);
            COLOR_ACTIVEOPTION_BKGRND = COLOR_BKGRND_LIST_HOVER;
            COLOR_ACTIVEOPTION_TEXT   = COLOR_MAIN_TEXT;

            COLOR_AUX_EDGE_NORMAL         = COLOR_BKGRND_AUX;
            COLOR_AUX_EDGE_HOVER          = COLOR_PROC(SOLAR_VIOLET);
            COLOR_AUX_EDGE_ACTIVE         = COLOR_PROC(SOLAR_CYAN);
            COLOR_AUX_TEXT                = COLOR_LIST_TEXT;
            COLOR_AUX_ACTIVEOPTION_BKGRND = COLOR_BKGRND_LIST_HOVER;
            COLOR_AUX_ACTIVEOPTION_TEXT   = COLOR_AUX_TEXT;

            COLOR_STATUS_ONLINE = COLOR_PROC(SOLAR_GREEN);
            COLOR_STATUS_AWAY   = COLOR_PROC(SOLAR_YELLOW);
            COLOR_STATUS_BUSY   = COLOR_PROC(SOLAR_RED);

            COLOR_BTN_SUCCESS_BKGRND        = COLOR_STATUS_ONLINE;
            COLOR_BTN_SUCCESS_TEXT          = COLOR_MAIN_TEXT;
            COLOR_BTN_SUCCESS_BKGRND_HOVER  = COLOR_PROC(SOLAR_CYAN);
            COLOR_BTN_SUCCESS_TEXT_HOVER    = COLOR_BKGRND_MAIN;
            COLOR_BTN_WARNING_BKGRND        = COLOR_STATUS_AWAY;
            COLOR_BTN_WARNING_TEXT          = COLOR_MAIN_TEXT;
            COLOR_BTN_WARNING_BKGRND_HOVER  = COLOR_PROC(SOLAR_ORANGE);
            COLOR_BTN_WARNING_TEXT_HOVER    = COLOR_BKGRND_MAIN;
            COLOR_BTN_DANGER_BACKGROUND     = COLOR_STATUS_BUSY;
            COLOR_BTN_DANGER_TEXT           = COLOR_MAIN_TEXT;
            COLOR_BTN_DANGER_BKGRND_HOVER   = COLOR_PROC(SOLAR_MAGENTA);
            COLOR_BTN_DANGER_TEXT_HOVER     = COLOR_BKGRND_MAIN;
            COLOR_BTN_DISABLED_BKGRND       = COLOR_PROC(SOLAR_BASE0);
            COLOR_BTN_DISABLED_TEXT         = COLOR_BKGRND_MAIN;
            COLOR_BTN_DISABLED_BKGRND_HOVER = COLOR_BKGRND_LIST_HOVER;
            COLOR_BTN_DISABLED_TRANSFER     = COLOR_BKGRND_LIST;
            COLOR_BTN_INPROGRESS_BKGRND     = COLOR_PROC(SOLAR_VIOLET);
            COLOR_BTN_INPROGRESS_TEXT       = COLOR_BKGRND_MAIN;

            COLOR_BTN_DISABLED_FORGRND   = COLOR_PROC(SOLAR_ORANGE);
            COLOR_BTN_INPROGRESS_FORGRND = COLOR_PROC(SOLAR_MAGENTA);
            break;

        case THEME_CUSTOM: {
            size_t   size;
            uint8_t *themedata = utox_data_load_custom_theme(&size);
            read_custom_theme(themedata, size);
        }
    }

    status_color[0] = COLOR_STATUS_ONLINE;
    status_color[1] = COLOR_STATUS_AWAY;
    status_color[2] = COLOR_STATUS_BUSY;
    status_color[3] = COLOR_STATUS_BUSY;
}

uint32_t *find_colour_pointer(char *color) {
    while (*color == 0 || *color == ' ' || *color == '\t')
        color++;

    int l = strlen(color) - 1;
    for (; l > 0; --l)
        if (color[l] != ' ' && color[l] != '\t')
            break;


    color[l + 1] = '\0';

    // remove "COLOR_" prefix
    if (!strncmp(color, "COLOR_", 6)) {
        color += 6;
    }

    debug("'%s'\n", color);

    for (int i = 0;; ++i) {
        const char *s = COLOUR_NAME_TABLE[i];
        if (!s)
            break;
        if (!strcmp(color, s))
            return COLOUR_POINTER_TABLE[i];
    }
    return NULL;
}

uint32_t try_parse_hex_colour(char *color, int *error) {
    while (*color == 0 || *color == ' ' || *color == '\t')
        color++;

    int l = strlen(color) - 1;
    for (; l > 0; --l)
        if (color[l] != ' ' && color[l] != '\n')
            break;

    color[++l] = '\0';

    debug("'%s'\n", color);
    if (l != 6) {
        *error = 1;
        return 0;
    }

    unsigned char red, green, blue;
    char          hex[3] = { 0 };

    memcpy(hex, color, 2);
    red = strtol(hex, NULL, 16);
    memcpy(hex, color + 2, 2);
    green = strtol(hex, NULL, 16);
    memcpy(hex, color + 4, 2);
    blue = strtol(hex, NULL, 16);

    return RGB(red, green, blue);
}

void read_custom_theme(const uint8_t *data, size_t length) {
    debug("Loading custom theme\n");

    while (length) {

        char *line = (char *)data;
        while (*line != 0) {
            if (*line == '#') {
                *line = 0;
                break;
            }
            ++line;
            --length;
        }

        char *color = strpbrk(line, "=");

        if (!color || color == line) {
            continue;
        }

        *color++ = 0;

        uint32_t *colorp = find_colour_pointer(line);
        if (!colorp) {
            continue;
        }

        int      err = 0;
        uint32_t col = try_parse_hex_colour(color, &err);

        if (err) {
            debug("error: parsing hex color failed\n");
            continue;
        } else {
            *colorp = COLOR_PROC(col);
            debug("set color...\n");
        }
    }

    return;
}
