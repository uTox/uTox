#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <stdint.h>

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

/* from settings.h */
typedef struct utox_save UTOX_SAVE;

/* from file_transfers.h */
typedef struct file_transfer FILE_TRANSFER;

/* from ui/panel.h */
typedef struct panel PANEL;

/* from ui/scrollable.h */
typedef struct scrollable SCROLLABLE;

/* from ui/button.h */
typedef struct button BUTTON;

/* from ui/switch.h */
typedef struct uiswitch UISWITCH;

/* from ui/edit.h */
typedef struct edit EDIT;

/* from ui/dropdown.h */
typedef struct dropdown DROPDOWN;

/* from windows/window.h, xlib/window.h */
typedef struct native_window UTOX_WINDOW;

/* from ui.h */
typedef struct maybe_i18nal_string MAYBE_I18NAL_STRING;

#endif
