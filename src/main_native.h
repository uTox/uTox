// Enums
/* uTox debug levels */
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

#ifndef FILE_TRANSFER_DEFINED
#define FILE_TRANSFER_DEFINED
typedef struct file_transfer FILE_TRANSFER;
#endif

void native_export_chatlog_init(uint32_t friend_number);

void native_select_dir_ft(uint32_t fid, uint32_t num, FILE_TRANSFER *file);

#endif
