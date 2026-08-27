#include "test.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/theme.c"
#include "../src/theme_tables.c"

bool test_parse_hex_ok(void) {
    bool err = false;
    char color[] = "FF00AA";
    uint32_t c = try_parse_hex_colour(color, &err);
    if (err) {
        FAIL("valid hex should not error");
    }
    if (c != RGB(0xFF, 0x00, 0xAA)) {
        FAIL("parsed colour mismatch %08x", c);
    }

    char padded[] = "  112233\n";
    err = false;
    c = try_parse_hex_colour(padded, &err);
    if (err || c != RGB(0x11, 0x22, 0x33)) {
        FAIL("padded hex failed err=%d c=%08x", err, c);
    }
    return true;
}

bool test_parse_hex_bad(void) {
    bool err = false;
    char short_hex[] = "FF00";
    try_parse_hex_colour(short_hex, &err);
    if (!err) {
        FAIL("short hex should error");
    }

    err = false;
    char long_hex[] = "FF00AABB";
    try_parse_hex_colour(long_hex, &err);
    if (!err) {
        FAIL("long hex should error");
    }

    err = false;
    char empty[] = "";
    try_parse_hex_colour(empty, &err);
    if (!err) {
        FAIL("empty hex should error");
    }

    err = false;
    try_parse_hex_colour(NULL, &err);
    if (!err) {
        FAIL("NULL hex should error");
    }

    err = false;
    char spaces[] = "   ";
    try_parse_hex_colour(spaces, &err);
    if (!err) {
        FAIL("spaces-only hex should error");
    }

    err = false;
    char tabs[] = "\t\t";
    try_parse_hex_colour(tabs, &err);
    if (!err) {
        FAIL("tabs-only hex should error");
    }

    err = false;
    char one[] = "A";
    try_parse_hex_colour(one, &err);
    if (!err) {
        FAIL("single char hex should error");
    }
    return true;
}

bool test_custom_theme_blob(void) {
    theme_load(THEME_DEFAULT);
    const uint32_t before = COLOR_MAIN_TEXT;

    char blob[] = "COLOR_MAIN_TEXT = 00FF00\n"
                  "# comment\n"
                  "COLOR_MAIN_URLTEXT=0011FF\n"
                  "UNKNOWN_KEY = AABBCC\n"
                  "\n"
                  "   \n"
                  "\t\r\n"
                  "=DEAD00\n"
                  "COLOR_MAIN_TEXT_SUBTEXT = GGGG00\n"
                  "MAIN_TEXT_HINT = ZZZZZZ\n"
                  "COLOR_MAIN_TEXT = FF00\n"
                  "COLOR_MAIN_TEXT_QUOTE\t\t= 008000\n"
                  "\0ignored after nul";

    read_custom_theme((const uint8_t *)blob, sizeof(blob) - 1);

    if (COLOR_MAIN_TEXT == before) {
        FAIL("COLOR_MAIN_TEXT should change after custom theme");
    }
    if (COLOR_MAIN_TEXT != RGB(0x00, 0xFF, 0x00)) {
        FAIL("COLOR_MAIN_TEXT expected %08x got %08x", RGB(0x00, 0xFF, 0x00), COLOR_MAIN_TEXT);
    }
    if (COLOR_MAIN_TEXT_URL != RGB(0x00, 0x11, 0xFF)) {
        FAIL("COLOR_MAIN_TEXT_URL expected %08x got %08x", RGB(0x00, 0x11, 0xFF), COLOR_MAIN_TEXT_URL);
    }

    char named[] = "  COLOR_MAIN_TEXT  ";
    if (!find_colour_pointer(named)) {
        FAIL("find_colour_pointer with COLOR_ prefix");
    }
    char bare[] = "MAIN_TEXT\t";
    if (!find_colour_pointer(bare)) {
        FAIL("find_colour_pointer without prefix");
    }
    char unknown[] = "NOT_A_COLOUR";
    if (find_colour_pointer(unknown)) {
        FAIL("unknown colour name should be NULL");
    }
    return true;
}

bool test_theme_load_builtins(void) {
    static const THEME builtins[] = {
        THEME_DEFAULT,
        THEME_LIGHT,
        THEME_DARK,
        THEME_HIGHCONTRAST,
        THEME_ZENBURN,
        THEME_SOLARIZED_LIGHT,
        THEME_SOLARIZED_DARK,
    };

    for (size_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        theme_load(builtins[i]);
        if (COLOR_BKGRND_MAIN == 0 && COLOR_MAIN_TEXT == 0) {
            FAIL("theme %u left colours unset", (unsigned)builtins[i]);
        }
    }

    theme_load(THEME_LIGHT);
    const uint32_t light_bg = COLOR_BKGRND_MAIN;
    theme_load(THEME_DARK);
    if (COLOR_BKGRND_MAIN == light_bg) {
        FAIL("dark and light backgrounds should differ");
    }
    return true;
}

bool test_theme_load_custom_file(void) {
    settings.portable_mode = true;
    native_create_dir((uint8_t *)"./tox/");

    FILE *fp = fopen("./tox/utox_theme.ini", "wb");
    if (!fp) {
        FAIL("write utox_theme.ini");
    }
    fputs("COLOR_MAIN_TEXT = 112233\n", fp);
    fclose(fp);

    theme_load(THEME_LIGHT);
    theme_load(THEME_CUSTOM);
    if (COLOR_MAIN_TEXT != RGB(0x11, 0x22, 0x33)) {
        remove("./tox/utox_theme.ini");
        FAIL("custom file theme not applied, got %08x", COLOR_MAIN_TEXT);
    }
    remove("./tox/utox_theme.ini");

    theme_load(THEME_LIGHT);
    const uint32_t light = COLOR_MAIN_TEXT;
    theme_load(THEME_CUSTOM); /* missing file: keep current colours */
    if (COLOR_MAIN_TEXT != light) {
        FAIL("missing custom theme file should not clobber colours");
    }

    /* Empty file: fread(size=0) fails. */
    fp = fopen("./tox/utox_theme.ini", "wb");
    if (!fp) {
        FAIL("write empty utox_theme.ini");
    }
    fclose(fp);
    theme_load(THEME_CUSTOM);
    remove("./tox/utox_theme.ini");
    return true;
}

int main(void) {
    int result = 0;
    settings.portable_mode = true;
    RUN_TEST(test_parse_hex_ok);
    RUN_TEST(test_parse_hex_bad);
    RUN_TEST(test_custom_theme_blob);
    RUN_TEST(test_theme_load_builtins);
    RUN_TEST(test_theme_load_custom_file);
    return result;
}
