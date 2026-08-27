#include "test.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/text.c"

bool test_utf8_len_and_validate(void) {
    if (utf8_len("a") != 1) {
        FAIL("ascii utf8_len");
    }

    const char *euro = "\xE2\x82\xAC"; /* € */
    if (utf8_len(euro) != 3) {
        FAIL("euro utf8_len");
    }

    char buf[] = "ab";
    if (utf8_unlen(buf + 1) != 1) {
        FAIL("utf8_unlen ascii");
    }

    uint8_t valid[] = "hello";
    if (utf8_validate(valid, 5) != 5) {
        FAIL("utf8_validate ascii");
    }

    /* Invalid continuation: keep leading bytes that form incomplete sequence truncated. */
    uint8_t bad[] = { 'a', 0xC3, 'b', 0 }; /* 0xC3 without continuation */
    int new_len = utf8_validate(bad, 3);
    if (new_len < 1 || bad[0] != 'a') {
        FAIL("utf8_validate should keep leading ascii");
    }

    uint8_t lone[] = { 0x80 };
    if (utf8_validate(lone, 1) != 0) {
        FAIL("lone continuation should stop immediately");
    }

    uint8_t allones[] = { 0xFF, 0x80 };
    if (utf8_validate(allones, 2) != 0) {
        FAIL("0xFF is not a valid utf8 lead");
    }

    uint8_t incomplete[] = { 0xC2 };
    if (utf8_validate(incomplete, 1) != 0) {
        FAIL("truncated 2-byte sequence");
    }

    uint8_t ok2[] = { 0xC2, 0xA9, 'z' };
    if (utf8_validate(ok2, 3) != 3) {
        FAIL("valid 2-byte then ascii");
    }

    uint8_t badcont[] = { 0xC2, 0x20 };
    if (utf8_validate(badcont, 2) != 0) {
        FAIL("2-byte with invalid continuation");
    }

    uint8_t startish[] = { 0xC2, 0xC0 };
    if (utf8_validate(startish, 2) != 0) {
        FAIL("continuation with 0x40 should stop");
    }

    if (utf8_validate(NULL, 5) != 0 || utf8_validate(NULL, 0) != 0 || utf8_validate(valid, 0) != 0) {
        FAIL("utf8_validate NULL/empty");
    }

    const char *four = "\xF0\x9F\x98\x80";
    if (utf8_len(four) != 4) {
        FAIL("utf8_len 4-byte");
    }
    if (utf8_len("\xC2\xA9") != 2) {
        FAIL("utf8_len 2-byte");
    }
    char five_len[] = { (char)0xF8, 0 };
    if (utf8_len(five_len) != 5) {
        FAIL("utf8_len 5-byte lead");
    }
    char six_len[] = { (char)0xFC, 0 };
    if (utf8_len(six_len) != 6) {
        FAIL("utf8_len 6-byte lead");
    }
    char ff_len[] = { (char)0xFF, 0 };
    if (utf8_len(ff_len) != 8) {
        FAIL("utf8_len 0xFF should count 8 bits, got %u", utf8_len(ff_len));
    }

    return true;
}

bool test_unicode_to_utf8(void) {
    char dst[8] = { 0 };
    if (unicode_to_utf8_len(0x20AC) != 3) {
        FAIL("euro unicode_to_utf8_len");
    }
    unicode_to_utf8(0x20AC, dst);
    if (memcmp(dst, "\xE2\x82\xAC", 3) != 0) {
        FAIL("unicode_to_utf8 euro");
    }

    char ascii[4] = { 0 };
    unicode_to_utf8('Z', ascii);
    if (ascii[0] != 'Z') {
        FAIL("unicode_to_utf8 ascii");
    }

    char two[4] = { 0 };
    if (unicode_to_utf8_len(0x00A9) != 2) { /* © */
        FAIL("2-byte unicode_to_utf8_len");
    }
    unicode_to_utf8(0x00A9, two);
    if ((uint8_t)two[0] != 0xC2 || (uint8_t)two[1] != 0xA9) {
        FAIL("unicode_to_utf8 2-byte");
    }

    char four[8] = { 0 };
    if (unicode_to_utf8_len(0x1F600) != 4) { /* 😀 */
        FAIL("4-byte unicode_to_utf8_len");
    }
    unicode_to_utf8(0x1F600, four);
    if ((uint8_t)four[0] != 0xF0) {
        FAIL("unicode_to_utf8 4-byte lead %02x", (unsigned)(uint8_t)four[0]);
    }
    if (unicode_to_utf8_len(0x220000) != 0) {
        FAIL("overlong unicode_to_utf8_len should be 0");
    }
    unicode_to_utf8(0x220000, dst); /* no-op beyond 21 bits */

    uint32_t ch = 0;
    char euro[] = "\xE2\x82\xAC";
    if (utf8_len_read(euro, &ch) != 3 || ch != 0x20AC) {
        FAIL("utf8_len_read euro got %u ch=%x", utf8_len_read(euro, &ch), ch);
    }
    if (utf8_len_read("A", &ch) != 1 || ch != 'A') {
        FAIL("utf8_len_read ascii");
    }
    const char *copyr = "\xC2\xA9";
    if (utf8_len_read(copyr, &ch) != 2 || ch != 0x00A9) {
        FAIL("utf8_len_read 2-byte");
    }
    if (utf8_unlen(euro + 3) != 3) {
        FAIL("utf8_unlen euro");
    }

    char fourb[] = "\xF0\x9F\x98\x80";
    if (utf8_len_read(fourb, &ch) != 4) {
        FAIL("utf8_len_read 4-byte");
    }
    char five[] = { (char)0xF8, 0x80, 0x80, 0x80, 0x80, 0 };
    if (utf8_len_read(five, &ch) != 5) {
        FAIL("utf8_len_read 5-byte");
    }
    char six[] = { (char)0xFC, 0x80, 0x80, 0x80, 0x80, 0x80, 0 };
    if (utf8_len_read(six, &ch) != 6) {
        FAIL("utf8_len_read 6-byte");
    }
    char invalid_lead[] = { (char)0xFF, 0 };
    if (utf8_len_read(invalid_lead, &ch) != 0) {
        FAIL("utf8_len_read 0xFF should return 0");
    }
    return true;
}

bool test_hex_and_case(void) {
    uint8_t in[] = { 0xAB, 0xCD, 0x00 };
    char out[8] = { 0 };
    to_hex(out, in, 2);
    if (strcmp(out, "ABCD") != 0 && strcmp(out, "abcd") != 0) {
        /* to_hex uses uppercase in uTox */
        if (memcmp(out, "ABCD", 4) != 0) {
            FAIL("to_hex got %s", out);
        }
    }

    if (memcmp_case("AbC", "aBc", 3) != 0) {
        FAIL("memcmp_case should match");
    }
    if (memcmp_case("AbC", "aBd", 3) == 0) {
        FAIL("memcmp_case should differ");
    }
    if (memcmp_case("ABC", "ABC", 3) != 0) {
        FAIL("memcmp_case uppercase");
    }
    if (memcmp_case("", "", 0) != 0) {
        FAIL("memcmp_case empty");
    }
    if (memcmp_case("{A", "{A", 2) != 0) {
        FAIL("memcmp_case brace is not a letter");
    }
    if (memcmp_case("{A", "{a", 2) != 0) {
        FAIL("memcmp_case brace then letter");
    }
    if (!strstr_case("Hello World", "WORLD")) {
        FAIL("strstr_case should find");
    }
    if (strstr_case("Hello", "xyz")) {
        FAIL("strstr_case should miss");
    }
    if (!strstr_case("zzHELLO", "hello")) {
        FAIL("strstr_case should recover after mismatch");
    }

    uint8_t low[] = { 0x01, 0x23 };
    char hex[8] = { 0 };
    to_hex(hex, low, 2);
    if (memcmp(hex, "0123", 4) != 0) {
        FAIL("to_hex low nibbles got %s", hex);
    }
    return true;
}

bool test_tohtml_and_shrink(void) {
    char *html = tohtml("<a>&", 4);
    if (!html) {
        FAIL("tohtml returned NULL");
    }
    if (!strstr(html, "&lt;") || !strstr(html, "&amp;")) {
        LOG("tohtml missing entities: %s", html);
        free(html);
        FAIL("tohtml missing entities");
    }
    free(html);

    html = tohtml("a>b", 3);
    if (!html || !strstr(html, "&gt;")) {
        free(html);
        FAIL("tohtml should encode greater-than");
    }
    free(html);

    html = tohtml("ok", 2);
    if (!html || strcmp(html, "ok") != 0) {
        free(html);
        FAIL("tohtml plain text");
    }
    free(html);

    html = tohtml("", 0);
    if (!html || html[0] != '\0') {
        free(html);
        FAIL("tohtml empty");
    }
    free(html);

    if (safe_shrink(NULL, 10, 4) != 0) {
        FAIL("safe_shrink NULL");
    }
    if (safe_shrink("hi", 2, 10) != 2) {
        FAIL("safe_shrink shorter than limit");
    }

    const char *s = "abcdef";
    if (safe_shrink(s, 6, 3) != 2) {
        FAIL("safe_shrink ascii expected 2 got %u", safe_shrink(s, 6, 3));
    }

    /* a + euro (3 bytes) + c */
    const char multi[] = { 'a', (char)0xE2, (char)0x82, (char)0xAC, 'c', 0 };
    uint16_t shrunk = safe_shrink(multi, 5, 3);
    if (shrunk != 1 && shrunk != 4) {
        /* either stop before euro (1) or include full euro (4) */
        FAIL("safe_shrink multi-byte got %u", shrunk);
    }
    return true;
}

bool test_sprint_humanread_bytes(void) {
    char dest[64];
    int n = sprint_humanread_bytes(dest, sizeof(dest), 1024);
    if (n <= 0 || !strstr(dest, "KiB")) {
        FAIL("sprint_humanread_bytes 1024: %s", dest);
    }
    n = sprint_humanread_bytes(dest, sizeof(dest), 10);
    if (n <= 0 || !strstr(dest, "B]")) {
        FAIL("sprint_humanread_bytes 10: %s", dest);
    }
    n = sprint_humanread_bytes(dest, sizeof(dest), 1024ull * 1024ull);
    if (n <= 0 || !strstr(dest, "MiB")) {
        FAIL("sprint_humanread_bytes 1MiB: %s", dest);
    }
    char tiny[4];
    n = sprint_humanread_bytes(tiny, sizeof(tiny), 1024);
    if (n <= 0) {
        FAIL("truncated sprint_humanread_bytes");
    }
    char two[2];
    n = sprint_humanread_bytes(two, sizeof(two), 10);
    if (n != 1) {
        FAIL("first snprintf overflow expected r=1 got %d", n);
    }
    char mid[6];
    n = sprint_humanread_bytes(mid, sizeof(mid), 1024);
    if (n <= 0) {
        FAIL("second snprintf overflow");
    }
    n = sprint_humanread_bytes(dest, sizeof(dest), UINT64_MAX);
    if (n <= 0) {
        FAIL("UINT64_MAX humanread");
    }
    return true;
}

int main(void) {
    int result = 0;
    RUN_TEST(test_utf8_len_and_validate);
    RUN_TEST(test_unicode_to_utf8);
    RUN_TEST(test_hex_and_case);
    RUN_TEST(test_tohtml_and_shrink);
    RUN_TEST(test_sprint_humanread_bytes);
    return result;
}
