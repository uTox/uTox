#ifndef NATIVE_KEYBOARD_H
#define NATIVE_KEYBOARD_H

// Push-to-talk

// Enable push-to-talk.
void init_ptt(void);

// Disable push-to-talk.
void exit_ptt(void);

// Returns a bool indicating whether you should send audio or not.
bool check_ptt_key(void);

// TODO: Make it possible to rebind push-to-talk key.
// Unimplemented.
bool get_ptt_key(void);
// Unimplemented.
bool set_ptt_key(void);


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
