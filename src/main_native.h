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

typedef struct file_transfer FILE_TRANSFER;


void native_export_chatlog_init(uint32_t friend_number);

void native_select_dir_ft(uint32_t fid, uint32_t num, FILE_TRANSFER *file);

bool native_move_file(const uint8_t *current_name, const uint8_t *new_name);

#endif
