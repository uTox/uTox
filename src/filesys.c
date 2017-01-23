#include "filesys.h"

#include "main_native.h"

bool utox_remove_file(const uint8_t *full_name, size_t length) {
    return native_remove_file(full_name, length);
}

bool utox_move_file(const uint8_t *current_name, const uint8_t *new_name) {
    return native_move_file(current_name, new_name);
}
