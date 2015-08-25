#include "main.h"
#include "theme_tables.h"

#define COLOR_PROC(a_ulColor) RGB((a_ulColor >> 16) & 0x0000FF, (a_ulColor >> 8) & 0x0000FF, a_ulColor & 0x0000FF)

void theme_load(char loadtheme)
{
    // Update the settings dropdown UI
    dropdown_theme.selected = loadtheme;
    theme = loadtheme;

    // ==== Default theme ====
    //---- Main chat area ----
    COLOR_BACKGROUND_MAIN = COLOR_PROC(0xffffff);
    COLOR_MAIN_TEXT       = COLOR_PROC(0x333333);
    COLOR_MAIN_CHATTEXT   = COLOR_PROC(0x000000);
    COLOR_MAIN_SUBTEXT    = COLOR_PROC(0x414141);
    COLOR_MAIN_ACTIONTEXT = COLOR_PROC(0x4e4ec8);
    COLOR_MAIN_QUOTETEXT  = COLOR_PROC(0x008000);
    COLOR_MAIN_REDTEXT    = COLOR_PROC(0xFF0000);
    COLOR_MAIN_URLTEXT    = COLOR_PROC(0x001fff);
    COLOR_MAIN_HINTTEXT   = COLOR_PROC(0x969696);

    //---- Friend list header and bottom-left buttons ----
    COLOR_BACKGROUND_MENU        = COLOR_PROC(0x1c1c1c);
    COLOR_MENU_TEXT              = COLOR_BACKGROUND_MAIN;
    COLOR_MENU_SUBTEXT           = COLOR_PROC(0xd1d1d1);
    COLOR_BACKGROUND_MENU_HOVER  = COLOR_PROC(0x282828);
    COLOR_BACKGROUND_MENU_ACTIVE = COLOR_PROC(0x414141);
    COLOR_MENU_ACTIVE_TEXT       = COLOR_BACKGROUND_MAIN;

    //---- Friend list  ----
    COLOR_BACKGROUND_LIST       = COLOR_PROC(0x414141);
    COLOR_BACKGROUND_LIST_HOVER = COLOR_PROC(0x505050);
    COLOR_LIST_TEXT             = COLOR_MENU_TEXT;
    COLOR_LIST_SUBTEXT          = COLOR_MENU_SUBTEXT;

    //---- Groupchat user list and title ----
    COLOR_GROUP_SELF           = COLOR_PROC(0x6bc260);
    COLOR_GROUP_PEER           = COLOR_MAIN_HINTTEXT;
    COLOR_GROUP_AUDIO          = COLOR_PROC(0xc84e4e);
    COLOR_GROUP_MUTED          = COLOR_MAIN_ACTIONTEXT;

    //---- Text selection ----
    COLOR_SELECTION_BACKGROUND = COLOR_MAIN_TEXT;
    COLOR_SELECTION_TEXT       = COLOR_BACKGROUND_MAIN;

    //---- Inputs, dropdowns & tooltips ----
    COLOR_EDGE_NORMAL             = COLOR_PROC(0xc0c0c0);
    COLOR_EDGE_HOVER              = COLOR_PROC(0x969696);
    COLOR_EDGE_ACTIVE             = COLOR_PROC(0x4ea6ea);
    COLOR_ACTIVEOPTION_BACKGROUND = COLOR_PROC(0xd1d1d1);
    COLOR_ACTIVEOPTION_TEXT       = COLOR_MAIN_TEXT;

    //---- Auxiliary style for inputs/dropdowns ("Search friends" bar) ----
    COLOR_BACKGROUND_AUX              = COLOR_PROC(0x313131);
    COLOR_AUX_EDGE_NORMAL             = COLOR_BACKGROUND_AUX;
    COLOR_AUX_EDGE_HOVER              = COLOR_PROC(0x999999);
    COLOR_AUX_EDGE_ACTIVE             = COLOR_PROC(0x1A73B7);
    COLOR_AUX_TEXT                    = COLOR_LIST_TEXT;
    COLOR_AUX_ACTIVEOPTION_BACKGROUND = COLOR_BACKGROUND_LIST_HOVER;
    COLOR_AUX_ACTIVEOPTION_TEXT       = COLOR_AUX_TEXT;

    //---- Status circles ----
    COLOR_STATUS_ONLINE = COLOR_PROC(0x6bc260);
    COLOR_STATUS_AWAY   = COLOR_PROC(0xcebf45);
    COLOR_STATUS_BUSY   = COLOR_PROC(0xc84e4e);

    //---- Buttons ----
    COLOR_BUTTON_SUCCESS_BACKGROUND       = COLOR_STATUS_ONLINE;
    COLOR_BUTTON_SUCCESS_TEXT             = COLOR_BACKGROUND_MAIN;
    COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND = COLOR_PROC(0x76d56a);
    COLOR_BUTTON_SUCCESS_HOVER_TEXT       = COLOR_BACKGROUND_MAIN;
    COLOR_BUTTON_WARNING_BACKGROUND       = COLOR_STATUS_AWAY;
    COLOR_BUTTON_WARNING_TEXT             = COLOR_BACKGROUND_MAIN;
    COLOR_BUTTON_WARNING_HOVER_BACKGROUND = COLOR_PROC(0xe3d24c);
    COLOR_BUTTON_WARNING_HOVER_TEXT       = COLOR_BACKGROUND_MAIN;
    COLOR_BUTTON_DANGER_BACKGROUND        = COLOR_STATUS_BUSY;
    COLOR_BUTTON_DANGER_TEXT              = COLOR_BACKGROUND_MAIN;
    COLOR_BUTTON_DANGER_HOVER_BACKGROUND  = COLOR_PROC(0xdc5656);
    COLOR_BUTTON_DANGER_HOVER_TEXT        = COLOR_BACKGROUND_MAIN;
    COLOR_BUTTON_DISABLED_BACKGROUND      = COLOR_PROC(0xd1d1d1);
    COLOR_BUTTON_DISABLED_TEXT            = COLOR_BACKGROUND_MAIN;
    COLOR_BUTTON_DISABLED_TRANSFER        = COLOR_BACKGROUND_LIST;
    COLOR_BUTTON_INPROGRESS_BACKGROUND    = COLOR_PROC(0x4ea6ea);
    COLOR_BUTTON_INPROGRESS_TEXT          = COLOR_BACKGROUND_MAIN;

    switch (loadtheme) {
    case THEME_DARK:
        COLOR_BACKGROUND_MAIN                 = COLOR_PROC(0x333333);
        COLOR_MAIN_TEXT                       = COLOR_PROC(0xdfdfdf);
        COLOR_MAIN_CHATTEXT                   = COLOR_PROC(0xffffff);
        COLOR_MAIN_SUBTEXT                    = COLOR_PROC(0xbbbbbb);
        COLOR_MAIN_ACTIONTEXT                 = COLOR_PROC(0x27a9bc);
        COLOR_MAIN_URLTEXT                    = COLOR_MAIN_ACTIONTEXT;
        COLOR_MAIN_QUOTETEXT                  = COLOR_PROC(0x55b317);
        COLOR_BACKGROUND_LIST                 = COLOR_PROC(0x222222);
        COLOR_BACKGROUND_LIST_HOVER           = COLOR_PROC(0x151515);
        COLOR_BACKGROUND_MENU                 = COLOR_PROC(0x171717);
        COLOR_BACKGROUND_MENU_HOVER           = COLOR_BACKGROUND_LIST_HOVER;
        COLOR_BACKGROUND_MENU_ACTIVE          = COLOR_BACKGROUND_LIST;
        COLOR_SELECTION_BACKGROUND            = COLOR_MAIN_TEXT;
        COLOR_SELECTION_TEXT                  = COLOR_BACKGROUND_MAIN;
        COLOR_GROUP_MUTED                     = COLOR_MAIN_URLTEXT;
        COLOR_EDGE_NORMAL                     = COLOR_PROC(0x555555);
        COLOR_EDGE_ACTIVE                     = COLOR_PROC(0x228888);
        COLOR_EDGE_HOVER                      = COLOR_PROC(0x999999);
        COLOR_ACTIVEOPTION_BACKGROUND         = COLOR_PROC(0x228888);
        COLOR_ACTIVEOPTION_TEXT               = COLOR_MAIN_TEXT;
        COLOR_BACKGROUND_AUX                  = COLOR_BACKGROUND_MENU;
        COLOR_AUX_EDGE_NORMAL                 = COLOR_BACKGROUND_AUX;
        COLOR_AUX_EDGE_ACTIVE                 = COLOR_EDGE_ACTIVE;
        COLOR_AUX_ACTIVEOPTION_BACKGROUND     = COLOR_ACTIVEOPTION_BACKGROUND;
        COLOR_MENU_ACTIVE_TEXT                = COLOR_MAIN_TEXT;
        COLOR_BUTTON_SUCCESS_BACKGROUND       = COLOR_PROC(0x414141);
        COLOR_BUTTON_SUCCESS_TEXT             = COLOR_PROC(0x33a63d);
        COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND = COLOR_PROC(0x455147);
        COLOR_BUTTON_SUCCESS_HOVER_TEXT       = COLOR_PROC(0x6eff3a);
        COLOR_BUTTON_WARNING_BACKGROUND       = COLOR_PROC(0x414141);
        COLOR_BUTTON_WARNING_TEXT             = COLOR_PROC(0xbd9e22);
        COLOR_BUTTON_WARNING_HOVER_BACKGROUND = COLOR_PROC(0x4c493c);
        COLOR_BUTTON_WARNING_HOVER_TEXT       = COLOR_PROC(0xff8d2a);
        COLOR_BUTTON_DANGER_BACKGROUND        = COLOR_PROC(0x414141);
        COLOR_BUTTON_DANGER_TEXT              = COLOR_PROC(0xbd2525);
        COLOR_BUTTON_DANGER_HOVER_BACKGROUND  = COLOR_PROC(0x513939);
        COLOR_BUTTON_DANGER_HOVER_TEXT        = COLOR_PROC(0xfa2626);
        COLOR_BUTTON_DISABLED_BACKGROUND      = COLOR_PROC(0x414141);
        COLOR_BUTTON_DISABLED_TEXT            = COLOR_MAIN_TEXT;
        COLOR_BUTTON_DISABLED_TRANSFER        = COLOR_BUTTON_DISABLED_TEXT;
        COLOR_BUTTON_INPROGRESS_BACKGROUND    = COLOR_BUTTON_DISABLED_BACKGROUND;
        COLOR_BUTTON_INPROGRESS_TEXT          = COLOR_MAIN_URLTEXT;
        break;

    case THEME_LIGHT:
        COLOR_BACKGROUND_LIST             = COLOR_PROC(0xf0f0f0);
        COLOR_BACKGROUND_LIST_HOVER       = COLOR_PROC(0xe0e0e0);
        COLOR_LIST_TEXT                   = COLOR_MAIN_TEXT;
        COLOR_LIST_SUBTEXT                = COLOR_MAIN_SUBTEXT;
        COLOR_BACKGROUND_MENU             = COLOR_BACKGROUND_LIST;
        COLOR_BACKGROUND_MENU_HOVER       = COLOR_PROC(0xe0e0e0);
        COLOR_BACKGROUND_MENU_ACTIVE      = COLOR_PROC(0x555555);
        COLOR_MENU_TEXT                   = COLOR_PROC(0x555555);
        COLOR_MENU_ACTIVE_TEXT            = COLOR_PROC(0xffffff);
        COLOR_MENU_SUBTEXT                = COLOR_PROC(0x414141);
        COLOR_EDGE_NORMAL                 = COLOR_PROC(0xc0c0c0);
        COLOR_EDGE_HOVER                  = COLOR_PROC(0x707070);
        COLOR_ACTIVEOPTION_BACKGROUND     = COLOR_PROC(0xc2e0ff);
        COLOR_ACTIVEOPTION_TEXT           = COLOR_MAIN_TEXT;
        COLOR_BACKGROUND_AUX              = COLOR_PROC(0xe0e0e0);
        COLOR_AUX_EDGE_NORMAL             = COLOR_BACKGROUND_AUX;
        COLOR_AUX_EDGE_HOVER              = COLOR_PROC(0x999999);
        COLOR_AUX_EDGE_ACTIVE             = COLOR_EDGE_ACTIVE;
        COLOR_AUX_TEXT                    = COLOR_LIST_TEXT;
        COLOR_AUX_ACTIVEOPTION_BACKGROUND = COLOR_ACTIVEOPTION_BACKGROUND;
        COLOR_AUX_ACTIVEOPTION_TEXT       = COLOR_AUX_TEXT;
        break;

    case THEME_HIGHCONTRAST:
        COLOR_BACKGROUND_MAIN                 = COLOR_PROC(0xffffff);
        COLOR_MAIN_TEXT                       = COLOR_PROC(0x000001);
        COLOR_MAIN_CHATTEXT                   = COLOR_MAIN_TEXT;
        COLOR_MAIN_SUBTEXT                    = COLOR_MAIN_TEXT;
        COLOR_MAIN_ACTIONTEXT                 = COLOR_PROC(0x0000ff);
        COLOR_MAIN_QUOTETEXT                  = COLOR_PROC(0x00ff00);
        COLOR_MAIN_URLTEXT                    = COLOR_MAIN_ACTIONTEXT;
        COLOR_MAIN_HINTTEXT                   = COLOR_MAIN_TEXT;
        COLOR_BACKGROUND_MENU                 = COLOR_BACKGROUND_MAIN;
        COLOR_MENU_TEXT                       = COLOR_MAIN_TEXT;
        COLOR_MENU_SUBTEXT                    = COLOR_MAIN_TEXT;
        COLOR_BACKGROUND_MENU_HOVER           = COLOR_BACKGROUND_MAIN;
        COLOR_BACKGROUND_MENU_ACTIVE          = COLOR_MAIN_TEXT;
        COLOR_MENU_ACTIVE_TEXT                = COLOR_BACKGROUND_MAIN;
        COLOR_BACKGROUND_LIST                 = COLOR_PROC(0x444444);
        COLOR_BACKGROUND_LIST_HOVER           = COLOR_MAIN_TEXT;
        COLOR_LIST_TEXT                       = COLOR_BACKGROUND_MAIN;
        COLOR_LIST_SUBTEXT                    = COLOR_BACKGROUND_MAIN;
        COLOR_GROUP_SELF                      = COLOR_PROC(0x00ff00);
        COLOR_GROUP_PEER                      = COLOR_MAIN_HINTTEXT;
        COLOR_GROUP_AUDIO                     = COLOR_PROC(0xff0000);
        COLOR_GROUP_MUTED                     = COLOR_MAIN_URLTEXT;
        COLOR_SELECTION_BACKGROUND            = COLOR_MAIN_TEXT;
        COLOR_SELECTION_TEXT                  = COLOR_BACKGROUND_MAIN;
        COLOR_EDGE_NORMAL                     = COLOR_MAIN_TEXT;
        COLOR_EDGE_ACTIVE                     = COLOR_MAIN_TEXT;
        COLOR_EDGE_HOVER                      = COLOR_MAIN_TEXT;
        COLOR_ACTIVEOPTION_BACKGROUND         = COLOR_MAIN_TEXT;
        COLOR_ACTIVEOPTION_TEXT               = COLOR_BACKGROUND_MAIN;
        COLOR_STATUS_ONLINE                   = COLOR_PROC(0x00ff00);
        COLOR_STATUS_AWAY                     = COLOR_PROC(0xffff00);
        COLOR_STATUS_BUSY                     = COLOR_PROC(0xff0000);
        COLOR_BUTTON_SUCCESS_BACKGROUND       = COLOR_PROC(0x00ff00);
        COLOR_BUTTON_SUCCESS_TEXT             = COLOR_BACKGROUND_MAIN;
        COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND = COLOR_PROC(0x00ff00);
        COLOR_BUTTON_SUCCESS_HOVER_TEXT       = COLOR_BACKGROUND_MAIN;
        COLOR_BUTTON_WARNING_BACKGROUND       = COLOR_PROC(0xffff00);
        COLOR_BUTTON_WARNING_TEXT             = COLOR_BACKGROUND_MAIN;
        COLOR_BUTTON_WARNING_HOVER_BACKGROUND = COLOR_PROC(0xffff00);
        COLOR_BUTTON_WARNING_HOVER_TEXT       = COLOR_BACKGROUND_MAIN;
        COLOR_BUTTON_DANGER_BACKGROUND        = COLOR_PROC(0xff0000);
        COLOR_BUTTON_DANGER_TEXT              = COLOR_BACKGROUND_MAIN;
        COLOR_BUTTON_DANGER_HOVER_BACKGROUND  = COLOR_PROC(0xff0000);
        COLOR_BUTTON_DANGER_HOVER_TEXT        = COLOR_BACKGROUND_MAIN;
        COLOR_BUTTON_DISABLED_BACKGROUND      = COLOR_PROC(0x444444);
        COLOR_BUTTON_DISABLED_TEXT            = COLOR_MAIN_TEXT;
        COLOR_BUTTON_DISABLED_TRANSFER        = COLOR_BACKGROUND_MAIN;
        COLOR_BUTTON_INPROGRESS_TEXT          = COLOR_BUTTON_DISABLED_TEXT;
        COLOR_BUTTON_INPROGRESS_BACKGROUND    = COLOR_PROC(0x00ffff);
        COLOR_BACKGROUND_AUX                  = COLOR_BACKGROUND_MAIN;
        COLOR_AUX_EDGE_NORMAL                 = COLOR_EDGE_NORMAL;
        COLOR_AUX_EDGE_HOVER                  = COLOR_EDGE_NORMAL;
        COLOR_AUX_EDGE_ACTIVE                 = COLOR_EDGE_ACTIVE;
        COLOR_AUX_TEXT                        = COLOR_MAIN_TEXT;
        COLOR_AUX_ACTIVEOPTION_BACKGROUND     = COLOR_ACTIVEOPTION_BACKGROUND;
        COLOR_AUX_ACTIVEOPTION_TEXT           = COLOR_ACTIVEOPTION_TEXT;
        break;

    case THEME_ZENBURN:
        COLOR_BACKGROUND_MAIN                  = COLOR_PROC(0x3f3f3f);
        COLOR_MAIN_TEXT                        = COLOR_PROC(0xdcdccc);
        COLOR_MAIN_CHATTEXT                    = COLOR_MAIN_TEXT;
        COLOR_MAIN_SUBTEXT                     = COLOR_MAIN_TEXT;
        COLOR_MAIN_ACTIONTEXT                  = COLOR_PROC(0xd0bf8f);
        COLOR_MAIN_QUOTETEXT                   = COLOR_PROC(0x7f9f7f);
        COLOR_MAIN_URLTEXT                     = COLOR_PROC(0x6ca0a3);
        COLOR_MAIN_HINTTEXT                    = COLOR_MAIN_TEXT;
        COLOR_BACKGROUND_MENU                  = COLOR_BACKGROUND_MAIN;
        COLOR_MENU_TEXT                        = COLOR_MAIN_TEXT;
        COLOR_MENU_SUBTEXT                     = COLOR_MAIN_TEXT;
        COLOR_BACKGROUND_MENU_HOVER            = COLOR_MAIN_QUOTETEXT;
        COLOR_BACKGROUND_MENU_ACTIVE           = COLOR_MAIN_QUOTETEXT;
        COLOR_MENU_ACTIVE_TEXT                 = COLOR_MAIN_TEXT;
        COLOR_BACKGROUND_LIST                  = COLOR_PROC(0x5f5f5f);
        COLOR_BACKGROUND_LIST_HOVER            = COLOR_PROC(0x7f7f7f);
        COLOR_LIST_TEXT                        = COLOR_MAIN_TEXT;
        COLOR_LIST_SUBTEXT                     = COLOR_MAIN_TEXT;
        COLOR_BACKGROUND_AUX                   = COLOR_BACKGROUND_MAIN;
        COLOR_AUX_EDGE_NORMAL                  = COLOR_BACKGROUND_LIST;
        COLOR_AUX_EDGE_HOVER                   = COLOR_MAIN_QUOTETEXT;
        COLOR_AUX_EDGE_ACTIVE                  = COLOR_MAIN_TEXT;
        COLOR_AUX_TEXT                         = COLOR_MAIN_TEXT;
        COLOR_AUX_ACTIVEOPTION_BACKGROUND      = COLOR_MAIN_QUOTETEXT;
        COLOR_AUX_ACTIVEOPTION_TEXT            = COLOR_MAIN_TEXT;
        COLOR_GROUP_SELF                       = COLOR_MAIN_TEXT;
        COLOR_GROUP_PEER                       = COLOR_MAIN_TEXT;
        COLOR_GROUP_AUDIO                      = COLOR_MAIN_QUOTETEXT;
        COLOR_GROUP_MUTED                      = COLOR_MAIN_ACTIONTEXT;
        COLOR_SELECTION_BACKGROUND             = COLOR_MAIN_QUOTETEXT;
        COLOR_SELECTION_TEXT                   = COLOR_MAIN_TEXT;
        COLOR_EDGE_NORMAL                      = COLOR_BACKGROUND_LIST;
        COLOR_EDGE_ACTIVE                      = COLOR_MAIN_TEXT;
        COLOR_EDGE_HOVER                       = COLOR_MAIN_QUOTETEXT;
        COLOR_ACTIVEOPTION_BACKGROUND          = COLOR_MAIN_QUOTETEXT;
        COLOR_ACTIVEOPTION_TEXT                = COLOR_MAIN_TEXT;
        COLOR_STATUS_ONLINE                    = COLOR_MAIN_QUOTETEXT;
        COLOR_STATUS_AWAY                      = COLOR_MAIN_ACTIONTEXT;
        COLOR_STATUS_BUSY                      = COLOR_PROC(0xcc9393);
        COLOR_BUTTON_SUCCESS_BACKGROUND        = COLOR_MAIN_QUOTETEXT;
        COLOR_BUTTON_SUCCESS_TEXT              = COLOR_MAIN_TEXT;
        COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND  = COLOR_PROC(0xbfebbf);
        COLOR_BUTTON_SUCCESS_HOVER_TEXT        = COLOR_PROC(0xffffff);
        COLOR_BUTTON_WARNING_BACKGROUND        = COLOR_MAIN_ACTIONTEXT;
        COLOR_BUTTON_WARNING_TEXT              = COLOR_BUTTON_SUCCESS_HOVER_TEXT;
        COLOR_BUTTON_WARNING_HOVER_BACKGROUND  = COLOR_PROC(0xf0dfaf);
        COLOR_BUTTON_WARNING_HOVER_TEXT        = COLOR_BUTTON_SUCCESS_HOVER_TEXT;
        COLOR_BUTTON_DANGER_BACKGROUND         = COLOR_STATUS_AWAY;
        COLOR_BUTTON_DANGER_TEXT               = COLOR_MAIN_TEXT;
        COLOR_BUTTON_DANGER_HOVER_BACKGROUND   = COLOR_PROC(0xdca3a3);
        COLOR_BUTTON_DANGER_HOVER_TEXT         = COLOR_BUTTON_SUCCESS_HOVER_TEXT;
        COLOR_BUTTON_DISABLED_BACKGROUND       = COLOR_BACKGROUND_LIST;
        COLOR_BUTTON_DISABLED_TEXT             = COLOR_MAIN_TEXT;
        COLOR_BUTTON_DISABLED_HOVER_BACKGROUND = COLOR_BACKGROUND_LIST;
        COLOR_BUTTON_DISABLED_TRANSFER         = COLOR_MAIN_TEXT;
        COLOR_BUTTON_INPROGRESS_BACKGROUND     = COLOR_MAIN_TEXT;
        COLOR_BUTTON_INPROGRESS_TEXT           = COLOR_BACKGROUND_MAIN;
        break;

    case THEME_CUSTOM: {
        uint8_t themepath[UTOX_FILE_NAME_LENGTH];
        int len = datapath(themepath);
        const char *s = "utox_theme.ini";
        int size = sizeof("utox_theme.ini");

        if (len + size > 1024) {
            puts("datapath too long, abandoning ship!");
            break;
        }

        memcpy(themepath + len, s, size);
        read_custom_theme((const char *)themepath);
    }
    }

    status_color[0] = COLOR_STATUS_ONLINE;
    status_color[1] = COLOR_STATUS_AWAY;
    status_color[2] = COLOR_STATUS_BUSY;
    status_color[3] = COLOR_STATUS_BUSY;
}

uint32_t *find_colour_pointer(char *colour) {
    while (*colour == 0 || *colour == ' ' || *colour == '\t')
        colour++;

    int l = strlen(colour) - 1;
    for (; l > 0; --l)
        if (colour[l] != ' ' && colour[l] != '\t')
            break;

    colour[l + 1] = '\0';
    debug("'%s'\n", colour);

    for (int i = 0;; ++i) {
        const char *s = COLOUR_NAME_TABLE[i];
        if (!s)
            break;
        if (!strcmp(colour, s))
            return COLOUR_POINTER_TABLE[i];
    }
    return NULL;
}

uint32_t try_parse_hex_colour(char *colour, int *error) {
    while (*colour == 0 || *colour == ' ' || *colour == '\t')
        colour++;

    int l = strlen(colour) - 1;
    for (; l > 0; --l)
        if (colour[l] != ' ' && colour[l] != '\n')
            break;

    colour[++l] = '\0';

    debug("'%s'\n", colour);
    if (l != 6) {
        *error = 1;
        return 0;
    }

    unsigned char red, green, blue;
    char hex[3] = { 0 };

    memcpy(hex, colour, 2);
    red = strtol(hex, NULL, 16);
    memcpy(hex, colour + 2, 2);
    green = strtol(hex, NULL, 16);
    memcpy(hex, colour + 4, 2);
    blue = strtol(hex, NULL, 16);

    return RGB(red, green, blue);
}

void read_custom_theme(const char *path) {
    puts("hello");
    puts(path);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("whad de fug DDD-xx");
        return;
    }

    char buf[1024]; // 1024 ought to be enough for anyone
    while (!feof(f)) {
        fgets(buf, 1024, f);

        char *line = buf;
        while (*line != 0) {
            if (*line == '#') {
                *line = 0;
                break;
            }
            line++;
        }

        char *colour = strpbrk(buf, "=");

        if (!colour || colour == buf)
            continue;

        *colour++ = 0;

        uint32_t *colourp = find_colour_pointer(buf);
        if (!colourp)
            continue;

        int err = 0;
        uint32_t col = try_parse_hex_colour(colour, &err);

        if (err) {
            puts("error");
            continue;
        } else {
            *colourp = COLOR_PROC(col);
            puts("set colour...");
        }
    }

    return;
}
