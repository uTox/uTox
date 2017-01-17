#include <stdint.h>
#include <string.h>

#include "../langs/i18n_decls.h"
#include "sized_string.h"
#include "util.h"

// FIXME: Required for UNUSED()
#include "main.h"

#ifdef msgid
#error "msgid is already defined"
#endif
#ifdef msgstr
#error "msgstr is already defined"
#endif
#ifdef LANG_POSIX_LOCALE
#error "LANG_POSIX_LOCALE is already defined"
#endif
#ifdef LANG_WINDOWS_ID
#error "LANG_WINDOWS_ID is already defined"
#endif
#ifdef LANG_PRIORITY
#error "LANG_PRIORITY is already defined"
#endif

/***** Parsing localized strings *****/

#define msgid(x) curr_id = (STR_##x);
#define msgstr(x)                                      \
    localized_strings[_LANG_ID][curr_id].str    = (x); \
    localized_strings[_LANG_ID][curr_id].length = sizeof(x) - 1;
#define LANG_WINDOWS_ID(x)
#define LANG_POSIX_LOCALE(x)
#define LANG_PRIORITY(x)

static STRING canary = STRING_INIT("BUG. PLEASE REPORT.");

static void init_strings(STRING (*localized_strings)[NUM_STRS]) {
    for (UTOX_LANG i = 0; i < NUM_LANGS; i++) {
        for (UTOX_I18N_STR j = 0; j < NUM_STRS; j++) {
            localized_strings[i][j] = canary;
        }
    }

    UTOX_I18N_STR curr_id = 0;

#include "ui_i18n.h"
}

#undef LANG_PRIORITY
#undef LANG_POSIX_LOCALE
#undef LANG_WINDOWS_ID
#undef msgstr
#undef msgid

STRING *ui_gettext(UTOX_LANG lang, UTOX_I18N_STR string_id) {
    static STRING localized_strings[NUM_LANGS][NUM_STRS];
    static int    ready = 0;

    if (!ready) {
        init_strings(localized_strings);
        ready = 1;
    }

    if ((lang >= NUM_LANGS) || (string_id >= NUM_STRS)) {
        return &canary;
    }

    return &localized_strings[lang][string_id];
}

/***** Parsing detection by POSIX locale *****/

#define msgid(x)
#define msgstr(x)
#define LANG_WINDOWS_ID(x)
#define LANG_POSIX_LOCALE(x) posix_locales[_LANG_ID] = (x);
#define LANG_PRIORITY(x) priorities[_LANG_ID]        = (x);

static void init_posix_locales(const char *UNUSED(posix_locales[]), int8_t UNUSED(priorities[])) {

#include "ui_i18n.h"
}

#undef LANG_PRIORITY
#undef LANG_POSIX_LOCALE
#undef LANG_WINDOWS_ID
#undef msgstr
#undef msgid

UTOX_LANG ui_guess_lang_by_posix_locale(const char *locale, UTOX_LANG deflt) {
    static const char *posix_locales[NUM_LANGS];
    static int8_t      priorities[NUM_LANGS];
    static int         ready = 0;

    if (!ready) {
        init_posix_locales(posix_locales, priorities);
        ready = 1;
    }

    UTOX_LANG found_lang = 0;
    int8_t    found_prio = INT8_MAX;

    // Try detecting by full prefix match first.
    for (UTOX_LANG i = 0; i < NUM_LANGS; i++) {
        const char *l = posix_locales[i];
        if (!l) {
            continue;
        }

        if (strstr(locale, l)) {
            if (found_prio > priorities[i]) {
                found_lang = i;
                found_prio = priorities[i];
            }
        }
    }

    if (found_prio < INT8_MAX) {
        return found_lang;
    }

    // It appears we haven't found exact language_territory
    // match (e.g. zh_TW) for given locale. ,
    // Try stripping territory off and search only by language part.
    for (UTOX_LANG i = 0; i < NUM_LANGS; i++) {
        const char *l = posix_locales[i];
        if (!l) {
            continue;
        }

        char *sep = strchr(l, '_');
        if (!sep) {
            continue;
        }

        if (!strncmp(locale, l, sep - l)) {
            if (found_prio > priorities[i]) {
                found_lang = i;
                found_prio = priorities[i];
            }
        }
    }

    return found_prio < INT8_MAX ? found_lang : deflt;
}

/***** Parsing detection by Windows language id *****/

#define msgid(x)
#define msgstr(x)
#define LANG_WINDOWS_ID(x) windows_lang_ids[_LANG_ID] = (x);
#define LANG_POSIX_LOCALE(x)
#define LANG_PRIORITY(x) priorities[_LANG_ID] = (x);

static void init_windows_lang_ids(uint16_t UNUSED(windows_lang_ids[]), int8_t UNUSED(priorities[])) {

#include "ui_i18n.h"
}

#undef LANG_PRIORITY
#undef LANG_POSIX_LOCALE
#undef LANG_WINDOWS_ID
#undef msgstr
#undef msgid

UTOX_LANG ui_guess_lang_by_windows_lang_id(uint16_t lang_id, UTOX_LANG deflt) {
    static uint16_t windows_lang_ids[NUM_LANGS];
    static int8_t   priorities[NUM_LANGS];
    static int      ready = 0;

    if (!ready) {
        init_windows_lang_ids(windows_lang_ids, priorities);
        ready = 1;
    }

    UTOX_LANG found_lang = 0;
    int8_t    found_prio = INT8_MAX;

    // Try detecting by full match first, including sublanguage part.
    for (UTOX_LANG i = 0; i < NUM_LANGS; i++) {
        uint16_t l = windows_lang_ids[i];
        if (!l) {
            continue;
        }

        if (l == lang_id) {
            if (found_prio > priorities[i]) {
                found_lang = i;
                found_prio = priorities[i];
            }
        }
    }

    if (found_prio < INT8_MAX) {
        return found_lang;
    }

    // It appears we haven't found exact id match.
    // Try matching by the lower 8 bits, which contain language family part.
    for (UTOX_LANG i = 0; i < NUM_LANGS; i++) {
        uint16_t l = windows_lang_ids[i];
        if (!l) {
            continue;
        }

        if ((l & 0xFF) == (lang_id & 0xFF)) {
            if (found_prio > priorities[i]) {
                found_lang = i;
                found_prio = priorities[i];
            }
        }
    }

    return found_prio < INT8_MAX ? found_lang : deflt;
}
