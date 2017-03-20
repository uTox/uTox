#include "updater.h"

#include "test.h"

#include <stdbool.h>

bool test_new_version(void);
bool test_no_update(void);

int main() {
    int result = 0;
    RUN_TEST(test_no_update);
    RUN_TEST(test_new_version);
    return result;
}

bool test_new_version(void) {
    LOG("Checking for update with an outdated version.");

#define VER_MAJOR 0
#define VER_MINOR 12
#define VER_PATCH 0
#define UTOX_VERSION_NUMBER (VER_MAJOR << 16 | VER_MINOR << 8 | VER_PATCH)

    if (updater_check()) {
        return true;
    }

    return false;
}

bool test_no_update(void) {
    LOG("Checking for update with the latest version.");
    if (!updater_check()) {
        return true;
    }

    return false;
}
