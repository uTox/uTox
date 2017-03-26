#ifndef NATIVE_KEYBOARD_H
#define NATIVE_KEYBOARD_H

// Push-to-talk

void init_ptt(void);

bool get_ptt_key(void); // Never used. Remove?

bool set_ptt_key(void); // Never used. Remove?

// Returns a bool indicating whether you should send audio or not.
bool check_ptt_key(void);

void exit_ptt(void);


// Native keycodes

#if defined __WIN32__
#include "win/keycodes.h"
#elif defined __ANDROID__
#include "android/keycodes.h"
#elif defined __OBJC__
#include "cocoa/keycodes.h"
#else
#include "xlib/keycodes.h"
#endif

#endif
