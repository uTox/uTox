#include <android/log.h>

#define debug(...) (__android_log_print(ANDROID_LOG_INFO, "utox", __VA_ARGS__))
