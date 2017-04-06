#ifndef NATIVE_COCOA_KEYCODES_H
#define NATIVE_COCOA_KEYCODES_H

#include <Carbon/Carbon.h>

#define CARBON_K(x) (x + 255)
#define KEY_BACK CARBON_K(kVK_Delete)
#define KEY_RETURN CARBON_K(kVK_Return)
#define KEY_LEFT CARBON_K(kVK_LeftArrow)
#define KEY_RIGHT CARBON_K(kVK_RightArrow)
#define KEY_TAB CARBON_K(kVK_Tab)
#define KEY_LEFT_TAB CARBON_K(kVK_ISO_Left_Tab)
#define KEY_DEL CARBON_K(kVK_ForwardDelete)
#define KEY_END CARBON_K(kVK_End)
#define KEY_HOME CARBON_K(kVK_Home)
#define KEY_UP CARBON_K(kVK_UpArrow)
#define KEY_DOWN CARBON_K(kVK_DownArrow)
#define KEY_PAGEUP CARBON_K(kVK_PageUp)
#define KEY_PAGEDOWN CARBON_K(kVK_PageDown)

#endif
