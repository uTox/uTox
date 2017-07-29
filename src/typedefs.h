#ifndef TYPEDEFS_H
#define TYPEDEFS_H

/* gather together all of the typedefs of incomplete structs,
 * so that they can be done once only, for compatibility with C99
 */

/* from android/main.h, windows/main.h, xlib/main.h cocoa/main.h*/
typedef struct native_image NATIVE_IMAGE;

/* there appears to be only one copy of this typedef:
./src/av/audio.h
typedef struct ALCdevice_struct ALCdevice;
*/

/* from groups.h */
typedef struct groupchat GROUPCHAT;

/* from messages.h */
typedef struct msg_header MSG_HEADER;

/* there appears to be only one copy of this typedef:
./src/chrono.h
typedef struct chrono_info CHRONO_INFO;
*/

/* from friend.h */
typedef struct utox_friend FRIEND;

/* from friend.h */
typedef struct utox_friend_request FREQUEST;

/* from friend.h and groups.h */
typedef unsigned int ALuint;

/* from avatar.h */
typedef struct avatar AVATAR;

/* from ui/edit.h */
typedef struct edit_change EDIT_CHANGE;

/* from friend.h, main.h, native/image.h, tox.h */
typedef uint8_t *UTOX_IMAGE;

#if 0

./src/friend.h
typedef struct file_transfer FILE_TRANSFER;
./src/layout/background.h
typedef struct panel PANEL;
./src/layout/friend.h
typedef struct scrollable SCROLLABLE;
./src/layout/friend.h
typedef struct panel PANEL;
./src/layout/friend.h
typedef struct button BUTTON;
./src/layout/friend.h
typedef struct uiswitch UISWITCH;
./src/layout/friend.h
typedef struct edit EDIT;
./src/layout/group.h
typedef struct scrollable SCROLLABLE;
./src/layout/group.h
typedef struct panel PANEL;
./src/layout/group.h
typedef struct button BUTTON;
./src/layout/group.h
typedef struct dropdown DROPDOWN;
./src/layout/group.h
typedef struct edit EDIT;
./src/layout/notify.h
typedef struct panel PANEL;
./src/layout/notify.h
typedef struct button BUTTON;
./src/layout/settings.h
typedef struct scrollable SCROLLABLE;
./src/layout/settings.h
typedef struct panel PANEL;
./src/layout/settings.h
typedef struct button BUTTON;
./src/layout/settings.h
typedef struct uiswitch UISWITCH;
./src/layout/settings.h
typedef struct dropdown DROPDOWN;
./src/layout/settings.h
typedef struct edit EDIT;
./src/layout/sidebar.h
typedef struct scrollable SCROLLABLE;
./src/layout/sidebar.h
typedef struct panel PANEL;
./src/layout/sidebar.h
typedef struct button BUTTON;
./src/layout/sidebar.h
typedef struct edit EDIT;
./src/layout/userbadge.h
typedef struct button BUTTON;
./src/layout/userbadge.h
typedef struct edit EDIT;
./src/native/filesys.h
typedef struct file_transfer FILE_TRANSFER;
./src/native/filesys.h
typedef struct file_transfer FILE_TRANSFER;
./src/native/os.h
typedef struct utox_save UTOX_SAVE;
./src/native/window.h
typedef struct native_window UTOX_WINDOW;
./src/native/window.h
typedef struct panel PANEL;
./src/notify.h
typedef struct native_window UTOX_WINDOW;
./src/settings.h
typedef struct utox_save UTOX_SAVE;
./src/stb_image.h
typedef struct
./src/stb_image.h
typedef struct
./src/stb_image.h
typedef struct
./src/stb_image.h
typedef struct
./src/stb_image.h
typedef struct
./src/stb_image.h
typedef struct
./src/stb_image.h
typedef struct
./src/stb_image.h
typedef struct
./src/stb_image.h
typedef struct
./src/stb_image.h
typedef struct
./src/stb_image.h
typedef struct
./src/stb_image.h
typedef struct
./src/stb_image.h
typedef struct
./src/stb_image.h
typedef struct
./src/stb_image_write.h
typedef struct
./src/ui/button.h
typedef struct button BUTTON;
./src/ui/edit.h
typedef struct scrollable SCROLLABLE;
./src/ui/edit.h
typedef struct edit EDIT;
./src/ui/panel.h
typedef struct panel PANEL;
./src/ui/panel.h
typedef struct scrollable SCROLLABLE;
./src/ui/switch.h
typedef struct uiswitch UISWITCH;
./src/ui/text.h
typedef struct scrollable SCROLLABLE;
./src/ui/tooltip.h
typedef struct maybe_i18nal_string MAYBE_I18NAL_STRING;
./src/ui.h
typedef struct panel PANEL;
./src/ui.h
typedef struct scrollable SCROLLABLE;
./src/window.h
typedef struct native_window UTOX_WINDOW;
./src/window.h
typedef struct panel PANEL;
./src/xlib/gtk.h
typedef struct file_transfer FILE_TRANSFER;
./src/xlib/gtk.h
typedef struct file_transfer FILE_TRANSFER;

#endif  /* #if 0 */

#endif
