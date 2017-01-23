#include <stdio.h>

#define debug(...) ((utox_verbosity() >= VERBOSITY_DEBUG) ? printf(__VA_ARGS__) : (0))
#define debug_info(...) ((utox_verbosity() >= VERBOSITY_INFO) ? printf(__VA_ARGS__) : (0))
#define debug_notice(...) ((utox_verbosity() >= VERBOSITY_NOTICE) ? printf(__VA_ARGS__) : (0))
#define debug_warning(...) ((utox_verbosity() >= VERBOSITY_WARNING) ? printf(__VA_ARGS__) : (0))
#define debug_error(...) ((utox_verbosity() >= VERBOSITY_ERROR) ? printf(__VA_ARGS__) : (0))
