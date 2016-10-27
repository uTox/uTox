#ifndef SVG_H
#define SVG_H

#include <stdbool.h>

/* Scroll bar rounded corners top and bottom */
#define BM_SCROLLHALF_WIDTH SCROLL_WIDTH
#define BM_SCROLLHALF_HEIGHT (SCROLL_WIDTH / 2)
/* No idea */
#define BM_STATUSAREA_WIDTH (UTOX_SCALE(10))
#define BM_STATUSAREA_HEIGHT (UTOX_SCALE(20))
/* Text button on the add a new friend page */
#define _BM_ADD_WIDTH 9
#define BM_ADD_WIDTH SCALE(18)
#define BM_STATUS_WIDTH SCALE(9)
#define BM_STATUS_NOTIFY_WIDTH SCALE(14)
#define BM_NMSG_WIDTH SCALE(18)

/* Standard large size button */
#define BM_LBUTTON_WIDTH (UTOX_SCALE(26))
#define BM_LBUTTON_HEIGHT (UTOX_SCALE(20))
/* Standard small size button */
#define BM_SBUTTON_WIDTH (UTOX_SCALE(26))
#define BM_SBUTTON_HEIGHT (UTOX_SCALE(10))

#define BM_SWITCH_WIDTH (SCALE(60))
#define BM_SWITCH_HEIGHT (SCALE(25))
#define BM_SWITCH_TOGGLE_WIDTH (SCALE(26))
#define BM_SWITCH_TOGGLE_HEIGHT (SCALE(21))

/* File transfer buttons */
#define BM_FT_WIDTH (UTOX_SCALE(125))
#define BM_FT_HEIGHT (UTOX_SCALE(26))
#define BM_FTM_WIDTH (UTOX_SCALE(113))
#define BM_FTB_WIDTH (UTOX_SCALE(13))
#define BM_FTB_HEIGHT (UTOX_SCALE(14))
#define BM_FT_CAP_WIDTH (UTOX_SCALE(15))
/* something to do with contacts? */
#define BM_CONTACT_WIDTH (UTOX_SCALE(20))
/* no idea */
#define _BM_LBICON_WIDTH 11
#define BM_LBICON_WIDTH (UTOX_SCALE(11))
#define _BM_LBICON_HEIGHT 10
#define BM_LBICON_HEIGHT (UTOX_SCALE(10))
/* small file transfer button maybe? */
#define BM_FB_WIDTH (UTOX_SCALE(6))
#define BM_FB_HEIGHT (UTOX_SCALE(5))

/* small button placements */
#define BM_CHAT_BUTTON_WIDTH (UTOX_SCALE(20))
#define BM_CHAT_BUTTON_HEIGHT (UTOX_SCALE(20))
/* camera box */
#define _BM_CHAT_BUTTON_OVERLAY_WIDTH (14)
#define BM_CHAT_BUTTON_OVERLAY_WIDTH (UTOX_SCALE(14))
#define _BM_CHAT_BUTTON_OVERLAY_HEIGHT (14)
#define BM_CHAT_BUTTON_OVERLAY_HEIGHT (UTOX_SCALE(14))
/* Large chat button */
#define BM_CHAT_SEND_WIDTH (UTOX_SCALE(28))
#define BM_CHAT_SEND_HEIGHT (UTOX_SCALE(20))
/* Chat speech bubble */
#define _BM_CHAT_SEND_OVERLAY_WIDTH (20)
#define BM_CHAT_SEND_OVERLAY_WIDTH (UTOX_SCALE(20))
#define _BM_CHAT_SEND_OVERLAY_HEIGHT (16)
#define BM_CHAT_SEND_OVERLAY_HEIGHT (UTOX_SCALE(16))

#define _BM_FILE_WIDTH (11)
#define BM_FILE_WIDTH (UTOX_SCALE(11))
#define _BM_FILE_HEIGHT (10)
#define BM_FILE_HEIGHT (UTOX_SCALE(10))

#define _BM_FILE_BIG_WIDTH (22)
#define BM_FILE_BIG_WIDTH (UTOX_SCALE(22))
#define _BM_FILE_BIG_HEIGHT (20)
#define BM_FILE_BIG_HEIGHT (UTOX_SCALE(20))

#define _BM_CI_WIDTH 10
#define BM_CI_WIDTH (UTOX_SCALE(10))

/* SVG Bitmap names. */
typedef enum {
    BM_ONLINE = 1,
    BM_AWAY,
    BM_BUSY,
    BM_OFFLINE,
    BM_STATUS_NOTIFY,

    BM_ADD,
    BM_GROUPS,
    BM_TRANSFER,
    BM_SETTINGS,
    BM_SETTINGS_THREE_BAR,

    BM_LBUTTON,
    BM_SBUTTON,

    BM_SWITCH,
    BM_SWITCH_TOGGLE,

    BM_CONTACT,
    BM_CONTACT_MINI,
    BM_GROUP,
    BM_GROUP_MINI,

    BM_FILE,
    BM_DECLINE,
    BM_CALL,
    BM_VIDEO,

    BM_FT,
    BM_FTM,
    BM_FTB1,
    BM_FTB2,
    BM_FT_CAP,

    BM_NO,
    BM_PAUSE,
    BM_RESUME,
    BM_YES,

    BM_SCROLLHALFTOP,
    BM_SCROLLHALFBOT,
    BM_SCROLLHALFTOP_SMALL,
    BM_SCROLLHALFBOT_SMALL,
    BM_STATUSAREA,

    BM_CHAT_BUTTON_LEFT,
    BM_CHAT_BUTTON_RIGHT,
    BM_CHAT_BUTTON_OVERLAY_SCREENSHOT,
    BM_CHAT_SEND,
    BM_CHAT_SEND_OVERLAY,
    BM_ENDMARKER,
} SVG_IMG;

void *svg_data;

bool svg_draw(bool needmemory);

#endif
