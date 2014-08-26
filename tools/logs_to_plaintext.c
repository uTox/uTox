#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void* file_raw(char *path, uint32_t *size)
{
    FILE *file;
    char *data;
    int len;

    file = fopen(path, "rb");
    if(!file) {
        printf("File not found (%s)\n", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    len = ftell(file);
    data = malloc(len);
    if(!data) {
        fclose(file);
        return NULL;
    }

    fseek(file, 0, SEEK_SET);

    if(fread(data, len, 1, file) != 1) {
        printf("Read error (%s)\n", path);
        fclose(file);
        free(data);
        return NULL;
    }

    fclose(file);

    printf("Read %u bytes (%s)\n", len, path);

    if(size) {
        *size = len;
    }
    return data;
}

int main(int argc, char *argv[])
{
    uint8_t *in, *p, *end;
    uint32_t in_size;
    FILE *out;

    if(argc != 3) {
        printf("usage: ./a.out <file in> <file out>\n");
        return 0;
    }

    in = file_raw(argv[1], &in_size);
    if(!in) {
        return 0;
    }

    out = fopen(argv[2], "w");
    if(!out) {
        free(in);
        return 0;
    }

    p = in;
    end = in + in_size;

    while(p < end) {
        uint64_t time;
        uint16_t namelen, length;
        time_t rawtime;
        char *timestr;

        memcpy(&time, p, 8);
        memcpy(&namelen, p + 8, 2);
        memcpy(&length, p + 10, 2);
        p += 16;

        rawtime = (time_t)time;
        timestr = ctime(&rawtime);

        if(p[12]) {
            fputs("[Sent]", out);
        }

        fprintf(out, "[%.*s] %.*s: %.*s\n", strlen(timestr) - 1, timestr, namelen, p, length, p + namelen);
        p += length + namelen;
    }

    free(in);
    fclose(out);
}
