#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

static uint64_t mock_now;

void yieldcpu(uint32_t ms) {
    usleep(ms * 1000);
}

uint64_t get_time(void) {
    return mock_now;
}

void mock_time_set(uint64_t now) {
    mock_now = now;
}

int file_lock(FILE *file, uint64_t start, size_t length) {
    (void)file;
    (void)start;
    (void)length;
    return 1;
}

int file_unlock(FILE *file, uint64_t start, size_t length) {
    (void)file;
    (void)start;
    (void)length;
    return 1;
}
