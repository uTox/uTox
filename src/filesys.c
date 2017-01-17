#include "filesys.h"

bool utox_remove_file(const uint8_t *full_name, size_t length) {
    return native_remove_file(full_name, length);
}
