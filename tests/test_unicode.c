#include <stdlib.h>
#include <check.h>
#include "../src/text.c"

START_TEST(test_utf8_strnlen)
{
	struct string {
		char *ptr;
		size_t exp_len;
	} str[] = {
		/* valid */
		[ 0] = { "", 0 },
		[ 1] = { "A", 0 },
		[ 2] = { "AB", 0 },
		[ 3] = { "ABCDEF", 0 },
		[ 4] = { "💩", 0 },
		[ 5] = { "💩💩", 0 },
		[ 6] = { "💩💩💩", 0 },
		[ 7] = { "A💩A", 0 },
		[ 8] = { "💩A💩", 0 },
		[ 9] = { "μTox", 0 },
		[10] = { "ㅇ", 0 },
		[11] = { "한", 0 },
		[12] = { "한극", 0 },
		[13] = { "한극어", 0 },

		[14] = { "\x00", 0 },             /* U+0000 */
		[15] = { "\x01", 1 },
		[16] = { "\x7f", 1 },             /* U+007F */
		[17] = { "\xc2\x80", 2 },         /* U+0080 */
		[18] = { "\xdf\xbf", 2 },         /* U+07FF */
		[19] = { "\xe0\xa0\x80", 3 },     /* U+0800 */
		[20] = { "\xef\xbf\xbf", 3 },     /* U+FFFF */
		[21] = { "\xf0\x90\x80\x80", 4 }, /* U+10000*/
		[22] = { "\xf4\x8f\xbf\xbf", 4 }, /* U+10FFFF */

		[23] = { "\xef\xbf\xbd", 3 }, /* U+FFFD replacement character */

		/* invalid */
		[24] = { "\xff", 0 },
		[25] = { "\xfe", 0 },
		[26] = { "\xfc", 0 },
		[27] = { "\xf8", 0 },
		[28] = { "\xf0", 0 },
		[29] = { "\xe0", 0 },
		[30] = { "\xc0", 0 },
		[31] = { "\x80", 0 },

		[32] = { "\xfd", 0 },
		[33] = { "\xf9", 0 },
		[34] = { "\xf1", 0 },
		[35] = { "\xe1", 0 },
		[36] = { "\xc1", 0 },
		[37] = { "\x81", 0 },

		[38] = { "\xc2", 0 },
		[39] = { "\xdf", 0 },
		[40] = { "\xe0\xa0", 0 },
		[41] = { "\xef\xbf", 0 },
		[42] = { "\xef", 0 },
		[43] = { "\xf4", 0 },

		[44] = { "\xf0\x80\x80\x80", 0 },
		[45] = { "\xf0\x80\x80", 0 },
		[46] = { "\xf0\x80", 0 },
		[47] = { "\xf0\xc0\xc0\xc0", 0 },
		[48] = { "\xf0\xc0\xc0", 0 },
		[49] = { "\xf0\xc0", 0 },

/*
 * TODO
 * "The UCS code values 0xd800–0xdfff (UTF-16 surrogates) as well as 0xfffe
and 0xffff (UCS noncharacters) should not appear in conforming UTF-8
streams. According to RFC 3629 no point above U+10FFFF should be used,
which limits characters to four bytes."
$ man 7 utf-8

https://en.wikipedia.org/wiki/Universal_Character_Set_characters#Non-characters
http://www.unicode.org/faq/private_use.html
 */
		[50] = { "\xef\xbf\x90", 0 }, /* U+FDD0 http://xkcd.com/380/ */

/* 		[28] = { "\xce\xbc", 2 },
 */

/* 		[28] = { "A\0AAA", 0 },
 */
	};
	size_t str_size = sizeof(str) / sizeof(struct string);

	size_t str_len;
	size_t ret;
	size_t i;

	for (i=0; i<24; ++i) {
		str[i].exp_len = strlen(str[i].ptr);
	}

	for (i=0; i<str_size; ++i) {
		str_len = strlen(str[i].ptr);
		printf("Checking str[%2zu]: %2zu:%2zu\t\"%s\"\n",
		       i, str_len, str[i].exp_len, str[i].ptr);
		ret = utf8_strnlen(str[i].ptr, str_len);
		ck_assert_int_eq(ret, str[i].exp_len);
	}
}
END_TEST

Suite *unicode_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("Unicode");
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_utf8_strnlen);
	suite_add_tcase(s, tc_core);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = unicode_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
