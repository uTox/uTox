#include <stdio.h>
#include <stdint.h>
#include "ui.h"

#ifdef STR
  #error "STR is already defined"
#endif
#ifdef msgid
  #error "msgid is already defined"
#endif
#ifdef msgstr
  #error "msgstr is already defined"
#endif

#define STR(x) { .str = (uint8_t*)x, .length = sizeof(x) - 1 }
#define msgid(x) curr_id = (STR_##x);
#define msgstr(x) \
    localized_strings[_LANG_ID][curr_id].str = (uint8_t*)(x); \
    localized_strings[_LANG_ID][curr_id].length = sizeof(x) - 1;

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

#undef msgstr
#undef msgid
#undef STR

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
