#include "filesys.h"
#include "filesys_native.h"

#include "debug.h"
#include "settings.h"

#include <stdlib.h>

FILE *utox_get_file(const uint8_t *name, size_t *size, UTOX_FILE_OPTS opts) {
    return native_get_file(name, size, opts, settings.portable_mode);
}

bool utox_remove_file(const uint8_t *full_name, size_t length) {
    return native_remove_file(full_name, length, settings.portable_mode);
}

bool utox_move_file(const uint8_t *current_name, const uint8_t *new_name) {
    return native_move_file(current_name, new_name);
}

void *file_raw(char *path, uint32_t *size) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        // LOG_TRACE(__FILE__, "File not found (%s)" , path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    int len = ftell(file);

    char *data = calloc(1, len);
    if (!data) {
        fclose(file);
        return NULL;
    }

    fseek(file, 0, SEEK_SET);

    if (fread(data, len, 1, file) != 1) {
        LOG_TRACE(__FILE__, "Read error (%s)" , path);
        fclose(file);
        free(data);
        return NULL;
    }

    fclose(file);

    if (size) {
        *size = len;
    }

    return data;
}
