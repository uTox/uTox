#include "logging_native.h"

#include "settings.h"

int utox_verbosity() {
    return settings.verbose;
}
