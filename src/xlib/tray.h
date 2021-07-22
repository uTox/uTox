// TODO fine the correct header for these, or consider an enum
#define SYSTEM_TRAY_REQUEST_DOCK 0
#define SYSTEM_TRAY_BEGIN_MESSAGE 1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

void create_tray_icon(void);

void destroy_tray_icon(void);

bool tray_window_event(XEvent *event);
