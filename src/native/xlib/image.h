#ifndef NATIVE_XLIB_IMAGE_H
#define NATIVE_XLIB_IMAGE_H

#include <X11/X.h>

#define NATIVE_IMAGE_IS_VALID(x) (None != (x))
#define NATIVE_IMAGE_HAS_ALPHA(x) (None != (x->alpha))

#endif
