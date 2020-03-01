#include "friend.h"
#include "messages.h"
#include "self.h"
#include "theme.h"
#include "theme_tables.h"
#include "tox.h"
#include "ui.h"

/* Globals */

/* friend.h */
uint8_t addfriend_status;

/* messages.h */
pthread_mutex_t messages_lock;

/* self.h */
struct utox_self self;

/* theme.h */
uint32_t COLOR_BKGRND_MAIN;
uint32_t COLOR_BKGRND_ALT;
uint32_t COLOR_BKGRND_AUX;
uint32_t COLOR_BKGRND_MENU;
uint32_t COLOR_BKGRND_MENU_HOVER;
uint32_t COLOR_BKGRND_MENU_ACTIVE;
uint32_t COLOR_BKGRND_LIST;
uint32_t COLOR_BKGRND_LIST_HOVER;

uint32_t COLOR_MAIN_TEXT;
uint32_t COLOR_MAIN_TEXT_CHAT;
uint32_t COLOR_MAIN_TEXT_SUBTEXT;
uint32_t COLOR_MAIN_TEXT_ACTION;
uint32_t COLOR_MAIN_TEXT_QUOTE;
uint32_t COLOR_MAIN_TEXT_RED;
uint32_t COLOR_MAIN_TEXT_URL;
uint32_t COLOR_MAIN_TEXT_HINT;

uint32_t COLOR_MSG_USER;
uint32_t COLOR_MSG_USER_PEND;
uint32_t COLOR_MSG_USER_ERROR;
uint32_t COLOR_MSG_CONTACT;

uint32_t COLOR_MENU_TEXT;
uint32_t COLOR_MENU_TEXT_SUBTEXT;
uint32_t COLOR_MENU_TEXT_ACTIVE;

uint32_t COLOR_LIST_TEXT;
uint32_t COLOR_LIST_TEXT_SUBTEXT;

uint32_t COLOR_AUX_EDGE_NORMAL;
uint32_t COLOR_AUX_EDGE_HOVER;
uint32_t COLOR_AUX_EDGE_ACTIVE;
uint32_t COLOR_AUX_TEXT;
uint32_t COLOR_AUX_ACTIVEOPTION_BKGRND;
uint32_t COLOR_AUX_ACTIVEOPTION_TEXT;

uint32_t COLOR_GROUP_SELF;
uint32_t COLOR_GROUP_PEER;
uint32_t COLOR_GROUP_AUDIO;
uint32_t COLOR_GROUP_MUTED;

uint32_t COLOR_SELECTION_BACKGROUND;
uint32_t COLOR_SELECTION_TEXT;

uint32_t COLOR_EDGE_NORMAL;
uint32_t COLOR_EDGE_ACTIVE;
uint32_t COLOR_EDGE_HOVER;

uint32_t COLOR_ACTIVEOPTION_BKGRND;
uint32_t COLOR_ACTIVEOPTION_TEXT;

uint32_t COLOR_STATUS_ONLINE;
uint32_t COLOR_STATUS_AWAY;

uint32_t COLOR_STATUS_BUSY;
uint32_t COLOR_BTN_SUCCESS_BKGRND;
uint32_t COLOR_BTN_SUCCESS_TEXT;
uint32_t COLOR_BTN_SUCCESS_BKGRND_HOVER;
uint32_t COLOR_BTN_SUCCESS_TEXT_HOVER;

uint32_t COLOR_BTN_WARNING_BKGRND;
uint32_t COLOR_BTN_WARNING_TEXT;
uint32_t COLOR_BTN_WARNING_BKGRND_HOVER;
uint32_t COLOR_BTN_WARNING_TEXT_HOVER;

uint32_t COLOR_BTN_DANGER_BACKGROUND;
uint32_t COLOR_BTN_DANGER_TEXT;
uint32_t COLOR_BTN_DANGER_BKGRND_HOVER;
uint32_t COLOR_BTN_DANGER_TEXT_HOVER;

uint32_t COLOR_BTN_DISABLED_BKGRND;
uint32_t COLOR_BTN_DISABLED_TEXT;
uint32_t COLOR_BTN_DISABLED_BKGRND_HOVER;
uint32_t COLOR_BTN_DISABLED_TRANSFER;

uint32_t COLOR_BTN_INPROGRESS_BKGRND;
uint32_t COLOR_BTN_INPROGRESS_TEXT;
uint32_t COLOR_BTN_DISABLED_FORGRND;
uint32_t COLOR_BTN_INPROGRESS_FORGRND;

uint32_t status_color[4];

/* theme_tables.h */
const char *COLOUR_NAME_TABLE[] = { "MAIN_BACKGROUND",
                                    "ALT_BACKGROUND",
                                    "MAIN_TEXT",
                                    "MAIN_CHATTEXT",
                                    "MAIN_SUBTEXT",
                                    "MAIN_ACTIONTEXT",
                                    "MAIN_QUOTETEXT",
                                    "MAIN_REDTEXT",
                                    "MAIN_URLTEXT",
                                    "MAIN_HINTTEXT",

                                    "MENU_BACKGROUND",
                                    "MENU_TEXT",
                                    "MENU_SUBTEXT",
                                    "MENU_BKGRND_HOVER",
                                    "MENU_ACTIVE_BACKGROUND",
                                    "MENU_ACTIVE_TEXT",

                                    "MSG_USER",
                                    "MSG_USER_PEND",
                                    "MSG_USER_ERROR",
                                    "MSG_CONTACT",

                                    "LIST_BACKGROUND",
                                    "LIST_BKGRND_HOVER",
                                    "LIST_TEXT",
                                    "LIST_SUBTEXT",

                                    "GROUP_SELF",
                                    "GROUP_PEER",
                                    "GROUP_AUDIO",
                                    "GROUP_MUTED",

                                    "SELECTION_BACKGROUND",
                                    "SELECTION_TEXT",

                                    "EDGE_NORMAL",
                                    "EDGE_HOVER",
                                    "EDGE_ACTIVE",
                                    "ACTIVEOPTION_BACKGROUND",
                                    "ACTIVEOPTION_TEXT",

                                    "AUX_BACKGROUND",
                                    "AUX_EDGE_NORMAL",
                                    "AUX_EDGE_HOVER",
                                    "AUX_EDGE_ACTIVE",
                                    "AUX_TEXT",
                                    "AUX_ACTIVEOPTION_BACKGROUND",
                                    "AUX_ACTIVEOPTION_TEXT",

                                    "STATUS_ONLINE",
                                    "STATUS_AWAY",
                                    "STATUS_BUSY",

                                    "BUTTON_SUCCESS_BACKGROUND",
                                    "BUTTON_SUCCESS_TEXT",
                                    "BUTTON_SUCCESS_BKGRND_HOVER",
                                    "BUTTON_SUCCESS_TEXT_HOVER",
                                    "BUTTON_WARNING_BACKGROUND",
                                    "BUTTON_WARNING_TEXT",
                                    "BUTTON_WARNING_BKGRND_HOVER",
                                    "BUTTON_WARNING_TEXT_HOVER",
                                    "BUTTON_DANGER_BACKGROUND",
                                    "BUTTON_DANGER_TEXT",
                                    "BUTTON_DANGER_BKGRND_HOVER",
                                    "BUTTON_DANGER_TEXT_HOVER",
                                    "BUTTON_DISABLED_BACKGROUND",
                                    "TRANSFER_PROGRESS_OVERLAY_PAUSED",
                                    "BUTTON_DISABLED_TEXT",
                                    "BUTTON_DISABLED_TRANSFER",
                                    "BUTTON_INPROGRESS_BACKGROUND",
                                    "TRANSFER_PROGRESS_OVERLAY_ACTIVE",
                                    "BUTTON_INPROGRESS_TEXT",
                                    NULL };
uint32_t *COLOUR_POINTER_TABLE[] = { &COLOR_BKGRND_MAIN,
                                     &COLOR_BKGRND_ALT,
                                     &COLOR_MAIN_TEXT,
                                     &COLOR_MAIN_TEXT_CHAT,
                                     &COLOR_MAIN_TEXT_SUBTEXT,
                                     &COLOR_MAIN_TEXT_ACTION,
                                     &COLOR_MAIN_TEXT_QUOTE,
                                     &COLOR_MAIN_TEXT_RED,
                                     &COLOR_MAIN_TEXT_URL,
                                     &COLOR_MAIN_TEXT_HINT,

                                     &COLOR_BKGRND_MENU,
                                     &COLOR_MENU_TEXT,
                                     &COLOR_MENU_TEXT_SUBTEXT,
                                     &COLOR_BKGRND_MENU_HOVER,
                                     &COLOR_BKGRND_MENU_ACTIVE,
                                     &COLOR_MENU_TEXT_ACTIVE,

                                     &COLOR_MSG_USER,
                                     &COLOR_MSG_USER_PEND,
                                     &COLOR_MSG_USER_ERROR,
                                     &COLOR_MSG_CONTACT,

                                     &COLOR_BKGRND_LIST,
                                     &COLOR_BKGRND_LIST_HOVER,
                                     &COLOR_LIST_TEXT,
                                     &COLOR_LIST_TEXT_SUBTEXT,

                                     &COLOR_GROUP_SELF,
                                     &COLOR_GROUP_PEER,
                                     &COLOR_GROUP_AUDIO,
                                     &COLOR_GROUP_MUTED,

                                     &COLOR_SELECTION_BACKGROUND,
                                     &COLOR_SELECTION_TEXT,

                                     &COLOR_EDGE_NORMAL,
                                     &COLOR_EDGE_HOVER,
                                     &COLOR_EDGE_ACTIVE,
                                     &COLOR_ACTIVEOPTION_BKGRND,
                                     &COLOR_ACTIVEOPTION_TEXT,

                                     &COLOR_BKGRND_AUX,
                                     &COLOR_AUX_EDGE_NORMAL,
                                     &COLOR_AUX_EDGE_HOVER,
                                     &COLOR_AUX_EDGE_ACTIVE,
                                     &COLOR_AUX_TEXT,
                                     &COLOR_AUX_ACTIVEOPTION_BKGRND,
                                     &COLOR_AUX_ACTIVEOPTION_TEXT,

                                     &COLOR_STATUS_ONLINE,
                                     &COLOR_STATUS_AWAY,
                                     &COLOR_STATUS_BUSY,

                                     &COLOR_BTN_SUCCESS_BKGRND,
                                     &COLOR_BTN_SUCCESS_TEXT,
                                     &COLOR_BTN_SUCCESS_BKGRND_HOVER,
                                     &COLOR_BTN_SUCCESS_TEXT_HOVER,
                                     &COLOR_BTN_WARNING_BKGRND,
                                     &COLOR_BTN_WARNING_TEXT,
                                     &COLOR_BTN_WARNING_BKGRND_HOVER,
                                     &COLOR_BTN_WARNING_TEXT_HOVER,
                                     &COLOR_BTN_DANGER_BACKGROUND,
                                     &COLOR_BTN_DANGER_TEXT,
                                     &COLOR_BTN_DANGER_BKGRND_HOVER,
                                     &COLOR_BTN_DANGER_TEXT_HOVER,
                                     &COLOR_BTN_DISABLED_BKGRND,
                                     &COLOR_BTN_DISABLED_FORGRND,
                                     &COLOR_BTN_DISABLED_TEXT,
                                     &COLOR_BTN_DISABLED_TRANSFER,
                                     &COLOR_BTN_INPROGRESS_BKGRND,
                                     &COLOR_BTN_INPROGRESS_FORGRND,
                                     &COLOR_BTN_INPROGRESS_TEXT,
                                     NULL };

/* tox.h */
UTOX_TOX_THREAD_INIT tox_thread_init;

TOX_MSG       tox_msg, audio_msg, toxav_msg;
volatile bool tox_thread_msg, audio_thread_msg, video_thread_msg;

bool tox_connected;
char proxy_address[256]; /* Magic Number inside toxcore */

/* ui.h */
struct utox_mouse mouse;

uint8_t cursor;
bool mdown;

char search_data[1024]; // TODO this is NOT where this belongs

double ui_scale;
