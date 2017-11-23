#include "filesys.h"
#include "native/filesys.h"

#include "debug.h"
#include "settings.h"

#include <stdlib.h>

FILE *utox_get_file(const char *name, size_t *size, UTOX_FILE_OPTS opts) {
    return native_get_file((uint8_t *)name, size, opts, settings.portable_mode);
}

FILE *utox_get_file_simple(const char *name, UTOX_FILE_OPTS opts) {
    return native_get_file_simple(name, opts);
}

bool utox_remove_file(const uint8_t *full_name, size_t length) {
    return native_remove_file(full_name, length, settings.portable_mode);
}

bool utox_move_file(const uint8_t *current_name, const uint8_t *new_name) {
    return native_move_file(current_name, new_name);
}

char *utox_get_filepath(const char *name) {
    return native_get_filepath(name);
}
