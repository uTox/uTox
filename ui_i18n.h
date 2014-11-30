
//"CZECH" "Čeština"
#define _LANG_ID LANG_CS
LANG_POSIX_LOCALE("cs_CZ")
LANG_WINDOWS_ID(0x0405)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/cs.h"
#undef _LANG_ID

//"BULGARIAN" "Български"
#define _LANG_ID LANG_BG
LANG_POSIX_LOCALE("bg_BG")
LANG_WINDOWS_ID(0x0402)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/bg.h"
#undef _LANG_ID

//"GERMAN" "Deutsch"
#define _LANG_ID LANG_DE
LANG_POSIX_LOCALE("de_DE")
LANG_WINDOWS_ID(0x0407)
LANG_PRIORITY(-1) // Ensure this lang gets chosen for unknown de locales.
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/de.h"
#undef _LANG_ID

//"ENGLISH" "English"
#define _LANG_ID LANG_EN
LANG_POSIX_LOCALE("en_US")
LANG_WINDOWS_ID(0x0409)
LANG_PRIORITY(-1) // Ensure this lang gets chosen for unknown en locales.
#include "langs/en.h"
#undef _LANG_ID

//"SPANISH" "Spanish"
#define _LANG_ID LANG_ES
LANG_POSIX_LOCALE("es_ES")
LANG_WINDOWS_ID(0x040A)
LANG_PRIORITY(-1) // Ensure this lang gets chosen for unknown es locales.
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/es.h"
#undef _LANG_ID

//"FRENCH" "Français"
#define _LANG_ID LANG_FR
LANG_POSIX_LOCALE("fr_FR")
LANG_WINDOWS_ID(0x040C)
LANG_PRIORITY(-1) // Ensure this lang gets chosen for unknown fr locales.
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/fr.h"
#undef _LANG_ID

//"HINDI" "Hindi"
#define _LANG_ID LANG_HI
LANG_POSIX_LOCALE("hi_IN")
LANG_WINDOWS_ID(0x0439)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/hi.h"
#undef _LANG_ID

//"JAPANESE" "日本語"
#define _LANG_ID LANG_JA
LANG_POSIX_LOCALE("ja_JP")
LANG_WINDOWS_ID(0x0411)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/ja.h"
#undef _LANG_ID

//"ITALIAN" "Italiano"
#define _LANG_ID LANG_IT
LANG_POSIX_LOCALE("it_IT")
LANG_WINDOWS_ID(0x0410)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/it.h"
#undef _LANG_ID

//"LATVIAN" "Latviešu"
#define _LANG_ID LANG_LV
LANG_POSIX_LOCALE("lv_LV")
LANG_WINDOWS_ID(0x0426)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/lv.h"
#undef _LANG_ID

//"DUTCH" "Nederlands"
#define _LANG_ID LANG_NL
LANG_POSIX_LOCALE("nl_NL")
LANG_WINDOWS_ID(0x0413)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/nl.h"
#undef _LANG_ID

//"NORWEGIAN" "Norsk"
#define _LANG_ID LANG_NO
LANG_POSIX_LOCALE("no_NO")
LANG_WINDOWS_ID(0x0414)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/no.h"
#undef _LANG_ID

//"POLISH" "Polski"
#define _LANG_ID LANG_PL
LANG_POSIX_LOCALE("pl_PL")
LANG_WINDOWS_ID(0x0415)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/pl.h"
#undef _LANG_ID

//"BRAZILIAN PORTUGUESE" "Português brasileiro"
#define _LANG_ID LANG_BR
LANG_POSIX_LOCALE("pt_BR")
LANG_WINDOWS_ID(0x0416)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/br.h"
#undef _LANG_ID

//"ROMANIAN" "Română"
#define _LANG_ID LANG_RO
LANG_POSIX_LOCALE("ro_RO")
LANG_WINDOWS_ID(0x0418)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/ro.h"
#undef _LANG_ID

//"RUSSIAN" "Русский"
#define _LANG_ID LANG_RU
LANG_POSIX_LOCALE("ru_RU")
LANG_WINDOWS_ID(0x0419)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/ru.h"
#undef _LANG_ID

//"TURKISH" "Türk"
#define _LANG_ID LANG_TR
LANG_POSIX_LOCALE("tr_TR")
LANG_WINDOWS_ID(0x041F)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/tr.h"
#undef _LANG_ID

//"UKRAINIAN" "Українська"
#define _LANG_ID LANG_UA
LANG_POSIX_LOCALE("uk_UA")
LANG_WINDOWS_ID(0x0422)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/ua.h"
#undef _LANG_ID

//"SIMPLIFIED CHINESE" "简体中文"
#define _LANG_ID LANG_CN
LANG_POSIX_LOCALE("zh_CN")
LANG_WINDOWS_ID(0x0804)
LANG_PRIORITY(-1) // Ensure this lang gets chosen for unknown zh locales.
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/cn.h"
#undef _LANG_ID

//"TRADITIONAL CHINESE" "繁體中文"
#define _LANG_ID LANG_TW
LANG_POSIX_LOCALE("zh_TW")
LANG_WINDOWS_ID(0x0404)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/tw.h"
#undef _LANG_ID

//"DANISH" "Dansk"
#define _LANG_ID LANG_DK
LANG_POSIX_LOCALE("da_DK")
LANG_WINDOWS_ID(0x0406)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/da.h"
#undef _LANG_ID

//"SWEDISH" "Svenska"
#define _LANG_ID LANG_SV
LANG_POSIX_LOCALE("sv_SE")
LANG_WINDOWS_ID(0x041d)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/sv.h"
#undef _LANG_ID

//"HUNGARIAN" "Magyar"
#define _LANG_ID LANG_HU
LANG_POSIX_LOCALE("hu_HU")
LANG_WINDOWS_ID(0x040E)
#include "langs/en.h" //fallback to English for untranslated things
#include "langs/hu.h"
#undef _LANG_ID
