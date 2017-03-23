#ifndef NATIVE_MAIN_H
#define NATIVE_MAIN_H

#if defined __WIN32__
#include "windows/main.h"
#elif defined __ANDROID__
#include "android/main.h"
#elif defined __OBJC__
#include "cocoa/main.h"
#else
#include "xlib/main.h"
#endif

typedef struct utox_save UTOX_SAVE;

/* OS-specific cleanup function for when edits are defocused. Commit IME state, etc. */
void edit_will_deactivate(void);

void showkeyboard(bool show);

void openurl(char *str);

void setselection(char *data, uint16_t length);

// inserts/deletes a value into the registry to launch uTox after boot
void launch_at_startup(int is_launch_at_startup);

void desktopgrab(bool video);

void config_osdefaults(UTOX_SAVE *r);

void openfilesend(void);

/* use the file chooser to pick an avatar and set it as the user's */
void openfileavatar(void);

#endif
