#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/filesys.h"
#include "../src/native/filesys.h"
#include "../src/settings.h"

bool test_filesys_append_delete_and_move(void) {
    settings.portable_mode = true;
    native_create_dir((uint8_t *)"./tox/");

    FILE *w = utox_get_file("fs_test.txt", NULL, UTOX_FILE_OPTS_WRITE);
    if (!w) {
        FAIL("write open");
    }
    fwrite("ab", 1, 2, w);
    fclose(w);

    FILE *a = utox_get_file("fs_test.txt", NULL, UTOX_FILE_OPTS_APPEND);
    if (!a) {
        FAIL("append open");
    }
    fwrite("cd", 1, 2, a);
    fclose(a);

    size_t size = 0;
    FILE *r = utox_get_file("fs_test.txt", &size, UTOX_FILE_OPTS_READ);
    if (!r || size != 4) {
        if (r) {
            fclose(r);
        }
        FAIL("read after append size=%zu", size);
    }
    char buf[8] = { 0 };
    if (fread(buf, 1, 4, r) != 4 || memcmp(buf, "abcd", 4) != 0) {
        fclose(r);
        FAIL("append contents");
    }
    fclose(r);

    utox_get_file("fs_test.txt", NULL, UTOX_FILE_OPTS_DELETE);
    FILE *gone = utox_get_file("fs_test.txt", NULL, UTOX_FILE_OPTS_READ);
    if (gone) {
        fclose(gone);
        FAIL("deleted file should not open for read");
    }

    FILE *src = fopen("./tox/move_src.txt", "wb");
    if (!src) {
        FAIL("create move src");
    }
    fwrite("z", 1, 1, src);
    fclose(src);
    if (!utox_move_file((const uint8_t *)"./tox/move_src.txt", (const uint8_t *)"./tox/move_dst.txt")) {
        FAIL("move file");
    }
    if (utox_move_file(NULL, (const uint8_t *)"./tox/x") || utox_move_file((const uint8_t *)"./tox/x", NULL)) {
        FAIL("move with NULL should fail");
    }
    remove("./tox/move_dst.txt");
    return true;
}

bool test_filesys_long_name_and_dir_tree(void) {
    settings.portable_mode = true;

    char long_name[UTOX_FILE_NAME_LENGTH];
    memset(long_name, 'a', sizeof long_name - 1);
    long_name[sizeof long_name - 1] = 0;
    FILE *fp = utox_get_file(long_name, NULL, UTOX_FILE_OPTS_WRITE);
    if (fp) {
        fclose(fp);
        FAIL("name that overflows path should fail");
    }

#ifndef _WIN32
    if (native_create_dir_tree("/") || native_create_dir_tree("")) {
        FAIL("short path should fail dir tree");
    }
    if (!native_create_dir_tree("./tox/sub/")) {
        FAIL("create nested tox/sub");
    }
#endif

    /* mkdir on a file path should fail (path occupied by a regular file). */
    FILE *blocker = fopen("./tox/notadir", "wb");
    if (blocker) {
        fwrite("x", 1, 1, blocker);
        fclose(blocker);
        if (native_create_dir((uint8_t *)"./tox/notadir")) {
            FAIL("mkdir on existing file path should fail");
        }
        remove("./tox/notadir");
    }

    char *path = utox_get_filepath("hello.txt");
    if (!path || !strstr(path, "hello.txt")) {
        free(path);
        FAIL("filepath should include name");
    }
    free(path);
    return true;
}

bool test_filesys_non_portable_home(void) {
    char tmp_home[512];
    snprintf(tmp_home, sizeof tmp_home, "./tox/fake_home_fs");
    native_create_dir((uint8_t *)"./tox/");
    native_create_dir((uint8_t *)tmp_home);
    {
        char cfg[576];
        snprintf(cfg, sizeof cfg, "%s/.config", tmp_home);
        native_create_dir((uint8_t *)cfg);
        snprintf(cfg, sizeof cfg, "%s/.config/tox", tmp_home);
        native_create_dir((uint8_t *)cfg);
    }

#ifndef _WIN32
    char *old_home = getenv("HOME");
    char *old_home_copy = old_home ? strdup(old_home) : NULL;
    setenv("HOME", tmp_home, 1);
#else
    char *old_home_copy = NULL;
    {
        char *h = getenv("HOME");
        if (h) {
            old_home_copy = strdup(h);
        }
        _putenv_s("HOME", tmp_home);
    }
#endif

    settings.portable_mode = false;
    FILE *fp = utox_get_file("np_test.txt", NULL, UTOX_FILE_OPTS_WRITE | UTOX_FILE_OPTS_MKDIR);
    if (!fp) {
        settings.portable_mode = true;
#ifndef _WIN32
        if (old_home_copy) {
            setenv("HOME", old_home_copy, 1);
        } else {
            unsetenv("HOME");
        }
#else
        if (old_home_copy) {
            _putenv_s("HOME", old_home_copy);
        } else {
            _putenv_s("HOME", "");
        }
#endif
        free(old_home_copy);
        FAIL("non-portable write under HOME");
    }
    fwrite("ok", 1, 2, fp);
    fclose(fp);

    char *path = utox_get_filepath("np_test.txt");
    if (!path || !strstr(path, ".config/tox")) {
        free(path);
        settings.portable_mode = true;
#ifndef _WIN32
        if (old_home_copy) {
            setenv("HOME", old_home_copy, 1);
        } else {
            unsetenv("HOME");
        }
#else
        if (old_home_copy) {
            _putenv_s("HOME", old_home_copy);
        } else {
            _putenv_s("HOME", "");
        }
#endif
        free(old_home_copy);
        FAIL("non-portable filepath should use .config/tox");
    }
    free(path);

    settings.portable_mode = true;
#ifndef _WIN32
    if (old_home_copy) {
        setenv("HOME", old_home_copy, 1);
    } else {
        unsetenv("HOME");
    }
#else
    if (old_home_copy) {
        _putenv_s("HOME", old_home_copy);
    } else {
        _putenv_s("HOME", "");
    }
#endif
    free(old_home_copy);
    return true;
}

bool test_native_ft_filename_safe(void) {
    uint8_t plain[] = "photo.png";
    if (!native_ft_filename_safe(plain, sizeof plain - 1)) {
        FAIL("plain name should be ok");
    }

    uint8_t slash[] = "a/b";
    if (native_ft_filename_safe(slash, sizeof slash - 1)) {
        FAIL("slash should reject");
    }

    uint8_t bslash[] = "a\\b";
    if (native_ft_filename_safe(bslash, sizeof bslash - 1)) {
        FAIL("backslash should reject");
    }

    uint8_t dot[] = ".";
    if (native_ft_filename_safe(dot, 1)) {
        FAIL(". should reject");
    }

    uint8_t dotdot[] = "..";
    if (native_ft_filename_safe(dotdot, 2)) {
        FAIL(".. should reject");
    }

    if (native_ft_filename_safe((uint8_t *)"", 0) || native_ft_filename_safe(NULL, 0)) {
        FAIL("empty/NULL should reject");
    }

    uint8_t embedded[] = { 'a', 0, 'b' };
    if (native_ft_filename_safe(embedded, sizeof embedded)) {
        FAIL("embedded NUL should reject");
    }

#ifdef _WIN32
    uint8_t win[] = "file?.txt";
    if (!native_ft_filename_safe(win, sizeof win - 1)) {
        FAIL("Windows should accept and replace ?");
    }
    if (win[4] != '_') {
        FAIL("Windows should replace ? with _");
    }

    uint8_t colon[] = "a:b.txt";
    if (!native_ft_filename_safe(colon, sizeof colon - 1)) {
        FAIL("Windows should accept and replace :");
    }
    if (colon[1] != '_') {
        FAIL("Windows should replace : with _");
    }

    uint8_t ctrl[] = { 'a', 1, 'b', 0 };
    if (native_ft_filename_safe(ctrl, 3)) {
        FAIL("Windows should reject control chars");
    }
#else
    uint8_t unix_ok[] = "file:name?.txt";
    if (!native_ft_filename_safe(unix_ok, sizeof unix_ok - 1)) {
        FAIL("Unix should allow : and ? in names");
    }
#endif

    return true;
}

int main(void) {
    int result = 0;
    settings.portable_mode = true;
    settings.verbose       = LOG_LVL_ERROR;
    RUN_TEST(test_filesys_append_delete_and_move);
    RUN_TEST(test_filesys_long_name_and_dir_tree);
    RUN_TEST(test_filesys_non_portable_home);
    RUN_TEST(test_native_ft_filename_safe);
    return result;
}
