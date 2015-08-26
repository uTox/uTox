/* Scroll bar rounded corners top and bottom */
#define  BM_SCROLLHALF_WIDTH SCROLL_WIDTH
#define  BM_SCROLLHALF_HEIGHT (SCROLL_WIDTH / 2)
/* No idea */
#define  BM_STATUSAREA_WIDTH (10 * SCALE)
#define  BM_STATUSAREA_HEIGHT (20 * SCALE)
/* Text button on the add a new friend page */
#define _BM_ADD_WIDTH           9
#define  BM_ADD_WIDTH           (9 * SCALE)
#define  BM_STATUS_WIDTH        (5 * SCALE)
#define  BM_STATUS_NOTIFY_WIDTH (7 * SCALE)
#define  BM_NMSG_WIDTH          (9 * SCALE)
/* Search and buttons toggle switch */
#define _BM_THREE_BAR_WIDTH 14
#define  BM_THREE_BAR_WIDTH (14 * SCALE)
/* Standard large size button */
#define  BM_LBUTTON_WIDTH  (26 * SCALE)
#define  BM_LBUTTON_HEIGHT (20 * SCALE)
/* Standard small size button */
#define  BM_SBUTTON_WIDTH  (26 * SCALE)
#define  BM_SBUTTON_HEIGHT (10 * SCALE)
/* File transfer buttons */
#define  BM_FT_WIDTH   (125 * SCALE)
#define  BM_FT_HEIGHT  (26 * SCALE)
#define  BM_FTM_WIDTH  (113 * SCALE)
#define  BM_FTB_WIDTH  (11 * SCALE)
#define  BM_FTB_HEIGHT (12 * SCALE)
/* something to do with contacts? */
#define  BM_CONTACT_WIDTH (20 * SCALE)
/* no idea */
#define _BM_LBICON_WIDTH  11
#define  BM_LBICON_WIDTH (11 * SCALE)
#define _BM_LBICON_HEIGHT 10
#define  BM_LBICON_HEIGHT (10 * SCALE)
/* no idea */
#define  BM_FB_WIDTH      (6 * SCALE)
#define  BM_FB_HEIGHT     (5 * SCALE)

/* small button placements */
#define  BM_CHAT_BUTTON_WIDTH          (20 * SCALE)
#define  BM_CHAT_BUTTON_HEIGHT         (20 * SCALE)
/* camera box */
#define _BM_CHAT_BUTTON_OVERLAY_WIDTH  (18        )
#define  BM_CHAT_BUTTON_OVERLAY_WIDTH  (18 * SCALE)
#define _BM_CHAT_BUTTON_OVERLAY_HEIGHT (18        )
#define  BM_CHAT_BUTTON_OVERLAY_HEIGHT (18 * SCALE)
/* Large chat button */
#define  BM_CHAT_SEND_WIDTH    (32 * SCALE)
#define  BM_CHAT_SEND_HEIGHT   (20 * SCALE)
/* Chat speech bubble */
#define _BM_CHAT_SEND_OVERLAY_WIDTH  (20        )
#define  BM_CHAT_SEND_OVERLAY_WIDTH  (20 * SCALE)
#define _BM_CHAT_SEND_OVERLAY_HEIGHT (16        )
#define  BM_CHAT_SEND_OVERLAY_HEIGHT (16 * SCALE)

#define _BM_CHAT_WIDTH 10
#define  BM_CHAT_WIDTH (10 * SCALE)

#define _BM_CI_WIDTH 10
#define  BM_CI_WIDTH (10 * SCALE)

void *svg_data;

_Bool svg_draw(_Bool needmemory);
