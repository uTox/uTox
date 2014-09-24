#include <stdint.h>
#include <string.h>

#include "sized_string.h"
#include "ui_i18n_decls.h"

#ifdef STR
  #error "STR is already defined"
#endif
#ifdef msgid
  #error "msgid is already defined"
#endif
#ifdef msgstr
  #error "msgstr is already defined"
#endif
#ifdef LANG_POSIX_LOCALE
  #error "LANG_POSIX_LOCALE is already defined"
#endif
#ifdef LANG_PRIORITY
  #error "LANG_PRIORITY is already defined"
#endif

#define STR(x) { .str = (uint8_t*)x, .length = sizeof(x) - 1 }

/***** Parsing localized strings *****/

#define msgid(x) curr_id = (STR_##x);
#define msgstr(x) \
    localized_strings[_LANG_ID][curr_id].str = (uint8_t*)(x); \
    localized_strings[_LANG_ID][curr_id].length = sizeof(x) - 1;
#define LANG_POSIX_LOCALE(x)
#define LANG_PRIORITY(x)

static STRING canary = STR("BUG. PLEASE REPORT.");

static void init_strings(STRING (*localized_strings)[STRS_MAX+1]) {
    uint16_t curr_id = 0;
    UI_LANG_ID i;
    UI_STRING_ID j;

    for(i = 0; i <= LANGS_MAX; i++) {
        for(j = 0; j <= STRS_MAX; j++) {
            localized_strings[i][j] = canary;
        }
    }

#include "ui_i18n.h"

}

#undef LANG_PRIORITY
#undef LANG_POSIX_LOCALE
#undef msgstr
#undef msgid

STRING* ui_gettext(UI_LANG_ID lang, UI_STRING_ID string_id) {
    static STRING localized_strings[LANGS_MAX+1][STRS_MAX+1];
    static int ready = 0;

    if(!ready) {
        init_strings(localized_strings);
        ready = 1;
    }

    if((lang > LANGS_MAX) || (string_id > STRS_MAX)) {
        return &canary;
    }

    return &localized_strings[lang][string_id];
}

/***** Parsing detection by POSIX locale *****/

#define msgid(x)
#define msgstr(x)
#define LANG_POSIX_LOCALE(x) posix_locales[_LANG_ID] = (x);
#define LANG_PRIORITY(x) priorities[_LANG_ID] = (x);

static void init_posix_locales(const char* posix_locales[], int8_t priorities[]) {

#include "ui_i18n.h"

}

#undef LANG_PRIORITY
#undef LANG_POSIX_LOCALE
#undef msgstr
#undef msgid

UI_LANG_ID ui_guess_lang_by_posix_locale(const char* locale, UI_LANG_ID deflt) {
    static const char* posix_locales[LANGS_MAX+1];
    static int8_t priorities[LANGS_MAX+1];
    static int ready = 0;

    if(!ready) {
        init_posix_locales(posix_locales, priorities);
        ready = 1;
    }

    UI_LANG_ID i, found_lang;
    int8_t found_prio = INT8_MAX;

    // Try detecting by full prefix match first.
    for(i = 0; i <= LANGS_MAX; i++) {
        const char* l = posix_locales[i];
        if(!l) continue;

        if(strstr(locale, l)) {
            if(found_prio > priorities[i]) {
                found_lang = i;
                found_prio = priorities[i];
            }
        }
    }

    if(found_prio < INT8_MAX) {
        return found_lang;
    }

    // It appears we haven't found exact language_territory 
    // match (e.g. zh_TW) for given locale. ,
    // Try stripping territory off and search only by language part.
    for(i = 0; i <= LANGS_MAX; i++) {
        const char* l = posix_locales[i];
        if(!l) continue;

        char* sep = strchr(l, '_');
        if(!sep) continue;

        if(!strncmp(locale, l, sep-l)) {
            if(found_prio > priorities[i]) {
                found_lang = i;
                found_prio = priorities[i];
            }
        }
    }

    return found_prio < INT8_MAX ? found_lang : deflt;
}
