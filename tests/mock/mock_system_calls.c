#include <stdint.h>
#include <unistd.h>

void yieldcpu(uint32_t ms) {
    usleep(ms * 1000);
}
