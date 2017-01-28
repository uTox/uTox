#include "filesys.h"

#include "main_native.h"
#include "debug.h"

#include <stdlib.h>

bool utox_remove_file(const uint8_t *full_name, size_t length) {
    return native_remove_file(full_name, length);
}

bool utox_move_file(const uint8_t *current_name, const uint8_t *new_name) {
    return native_move_file(current_name, new_name);
}

void *file_raw(char *path, uint32_t *size) {
    FILE *file;
    char *data;
    int   len;

    file = fopen(path, "rb");
    if (!file) {
        // LOG_TRACE(__FILE__, "File not found (%s)" , path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    len  = ftell(file);
    data = malloc(len);
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

    // LOG_TRACE(__FILE__, "Read %u bytes (%s)" , len, path);

    if (size) {
        *size = len;
    }
    return data;
}
