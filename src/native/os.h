#ifndef NATIVE_OS_H
#define NATIVE_OS_H

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

// Linux, OS X, and Windows.
void config_osdefaults(UTOX_SAVE *r);

#endif
