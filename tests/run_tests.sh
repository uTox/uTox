#!/bin/sh -e

cd "@utoxTESTS_BINARY_DIR@"

# AddressSanitizer / LeakSanitizer defaults for Linux ASAN builds.
# halt_on_error=1: a heap/stack bug must fail the test, not print-and-continue.
# Override from the environment if needed.
if [ -z "${ASAN_OPTIONS+x}" ]; then
    export ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1:detect_stack_use_after_return=1:exitcode=1
fi
if [ -z "${LSAN_OPTIONS+x}" ]; then
    export LSAN_OPTIONS=print_suppressions=0
fi

# remove ./tox folder before each test to have clean environment
rm -rf ./tox
ctest -VV
