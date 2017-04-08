#include "../src/updater.c"

#include "test.h"

#define UTOX_VERSION_NUMBER (VER_MAJOR << 16 | VER_MINOR << 8 | VER_PATCH)


START_TEST (test_updater_newer)
{
#undef VER_MAJOR
#undef VER_MINOR
#undef VER_PATCH
#define VER_MAJOR 0
#define VER_MINOR 10
#define VER_PATCH 0
    uint32_t v = updater_check();
    ck_assert_msg(v >= UTOX_VERSION_NUMBER, "Updater thinks it's version is newer");
}
END_TEST

START_TEST (test_updater_same)
{
#undef VER_MAJOR
#undef VER_MINOR
#undef VER_PATCH
#define VER_MAJOR 0
#define VER_MINOR 14
#define VER_PATCH 0
    uint32_t v = updater_check();
    ck_assert_msg(v == UTOX_VERSION_NUMBER, "Updater thinks it's version is different");
}
END_TEST

START_TEST (test_updater_older)
{
#undef VER_MAJOR
#undef VER_MINOR
#undef VER_PATCH
#define VER_MAJOR 255u
#define VER_MINOR 0
#define VER_PATCH 0
    uint32_t v = updater_check();
    ck_assert_msg(v <= UTOX_VERSION_NUMBER, "Updater thinks it's version is newer");
}
END_TEST


static Suite *suite(void)
{
    Suite *s = suite_create("uTox Updater");

    MK_TEST_CASE(updater_newer);
    MK_TEST_CASE(updater_same);
    MK_TEST_CASE(updater_older);

    return s;
}

int main(int argc, char *argv[])
{
    srand((unsigned int) time(NULL));

    Suite *run = suite();
    SRunner *test_runner = srunner_create(run);

    int number_failed = 0;
    srunner_run_all(test_runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(test_runner);

    srunner_free(test_runner);

    return number_failed;
}
