// Uncomment when the native function cleanup is done.
// #if defined(NATIVE_MAIN_H)
// #error "The main function should only ever be included once."
// #endif

#ifndef NATIVE_MAIN_H
#define NATIVE_MAIN_H

#if defined __WIN32__
#include "../windows/main.h"
#elif defined __ANDROID__
#include "../android/main.h"
#elif defined __OBJC__
#include "../cocoa/main.h"
#else
#include "../xlib/main.h"
#endif

typedef struct utox_save UTOX_SAVE;

// OS-specific cleanup function for when edits are defocused. Commit IME state, etc.
// OS X only.
void edit_will_deactivate(void);

// Android only.
void showkeyboard(bool show);

// Linux, OS X, and Windows.
void openurl(char *str);

// Linux only.
void setselection(char *data, uint16_t length);

// inserts/deletes a value into the registry to launch uTox after boot
// OS X and Windows
void launch_at_startup(int is_launch_at_startup);

// OS X only.
void desktopgrab(bool video);

// Linux, OS X, and Windows.
void config_osdefaults(UTOX_SAVE *r);

// Linux, OS X, and Windows.
void openfilesend(void);

// Use the file picker to select an avatar and set it as the user's.
// Linux, OS X, and Windows.
void openfileavatar(void);

#endif
