
//"CZECH" "Čeština"
#define _LANG_ID LANG_CS
LANG_POSIX_LOCALE("cs_CZ")
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/cs.h"
#undef _LANG_ID

//"BULGARIAN" "Български"
#define _LANG_ID LANG_BG
LANG_POSIX_LOCALE("bg_BG")
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/bg.h"
#undef _LANG_ID

//"GERMAN" "Deutsch"
#define _LANG_ID LANG_DE
LANG_POSIX_LOCALE("de_DE")
LANG_PRIORITY(-1) // Ensure this lang gets chosen for unknown de locales.
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/de.h"
#undef _LANG_ID

//"ENGLISH" "English"
#define _LANG_ID LANG_EN
LANG_POSIX_LOCALE("en_US")
LANG_PRIORITY(-1) // Ensure this lang gets chosen for unknown en locales.
#include "langs/en.h"
#undef _LANG_ID

//"SPANISH" "Spanish"
#define _LANG_ID LANG_ES
LANG_POSIX_LOCALE("es_ES")
LANG_PRIORITY(-1) // Ensure this lang gets chosen for unknown es locales.
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/es.h"
#undef _LANG_ID

//"FRENCH" "Français"
#define _LANG_ID LANG_FR
LANG_POSIX_LOCALE("fr_FR")
LANG_PRIORITY(-1) // Ensure this lang gets chosen for unknown fr locales.
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/fr.h"
#undef _LANG_ID

//"HINDI" "Hindi"
#define _LANG_ID LANG_HI
LANG_POSIX_LOCALE("hi_IN")
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/hi.h"
#undef _LANG_ID

//"JAPANESE" "日本語"
#define _LANG_ID LANG_JA
LANG_POSIX_LOCALE("ja_JP")
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/ja.h"
#undef _LANG_ID

//"ITALIAN" "Italiano"
#define _LANG_ID LANG_IT
LANG_POSIX_LOCALE("it_IT")
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/it.h"
#undef _LANG_ID

//"LATVIAN" "Latviešu"
#define _LANG_ID LANG_LV
LANG_POSIX_LOCALE("lv_LV")
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/lv.h"
#undef _LANG_ID

//"DUTCH" "Nederlands"
#define _LANG_ID LANG_NL
LANG_POSIX_LOCALE("nl_NL")
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/nl.h"
#undef _LANG_ID

//"NORWEGIAN" "Norsk"
#define _LANG_ID LANG_NO
LANG_POSIX_LOCALE("no_NO")
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/no.h"
#undef _LANG_ID

//"POLISH" "Polski"
#define _LANG_ID LANG_PL
LANG_POSIX_LOCALE("pl_PL")
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/pl.h"
#undef _LANG_ID

//"ROMANIAN" "Română"
#define _LANG_ID LANG_RO
LANG_POSIX_LOCALE("ro_RO")
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/ro.h"
#undef _LANG_ID

//"RUSSIAN" "Русский"
#define _LANG_ID LANG_RU
LANG_POSIX_LOCALE("ru_RU")
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/ru.h"
#undef _LANG_ID

//"TURKISH" "Türk"
#define _LANG_ID LANG_TR
LANG_POSIX_LOCALE("tr_TR")
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/tr.h"
#undef _LANG_ID

//"UKRAINIAN" "Українська"
#define _LANG_ID LANG_UA
LANG_POSIX_LOCALE("uk_UA")
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/ua.h"
#undef _LANG_ID

//"SIMPLIFIED CHINESE" "简体中文"
#define _LANG_ID LANG_CN
LANG_POSIX_LOCALE("zh_CN")
LANG_PRIORITY(-1) // Ensure this lang gets chosen for unknown zh locales.
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/cn.h"
#undef _LANG_ID

//"TRADITIONAL CHINESE" "繁體中文"
#define _LANG_ID LANG_TW
LANG_POSIX_LOCALE("zh_TW")
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/tw.h"
#undef _LANG_ID

