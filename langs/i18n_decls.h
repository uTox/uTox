#ifndef I18N_DECLS_H
#define I18N_DECLS_H

typedef struct sized_string STRING;

#include <stdint.h>

typedef enum {
    LANG_BG,
    LANG_DE,
    LANG_EN,
    LANG_ES,
    LANG_FR,
    LANG_HI, // 5
    LANG_JA,
    LANG_IT,
    LANG_LV,
    LANG_NL,
    LANG_NO, // 10
    LANG_BR,
    LANG_PL,
    LANG_RO,
    LANG_RU,
    LANG_TR, // 15
    LANG_UA,
    LANG_CN,
    LANG_TW,
    LANG_CS,
    LANG_DK,
    LANG_SV,
    LANG_HU,
    LANG_PT,
    LANG_EO,
    LANG_HR,

    NUM_LANGS // add langs before this line
} UTOX_LANG;

typedef enum {
    // This ensures that all statically initialized to zero UTOX_I18N_STR vars
    // will render as canary "BUG. PLEASE REPORT." strings.
    UI_STRING_ID_INVALID = 0,

    STR_LANG_NATIVE_NAME,
    STR_LANG_ENGLISH_NAME,

    STR_SPLASH_TITLE,
    STR_SPLASH_TEXT,

    STR_CHANGE_LOG_TITLE,
    STR_CHANGE_LOG_TEXT,

    STR_REQ_SENT,
    STR_REQ_RESOLVE,
    STR_DNS_DISABLED,
    STR_REQ_INVALID_ID,
    STR_REQ_EMPTY_ID,
    STR_REQ_LONG_MSG,
    STR_REQ_NO_MSG,
    STR_REQ_SELF_ID,
    STR_REQ_ALREADY_FRIENDS,
    STR_REQ_UNKNOWN,
    STR_REQ_BAD_CHECKSUM,
    STR_REQ_BAD_NOSPAM,
    STR_REQ_NO_MEMORY,

    STR_SEND_FILE,
    STR_SAVE_FILE,
    STR_WHERE_TO_SAVE_FILE_PROMPT,
    STR_WHERE_TO_SAVE_FILE,
    STR_SEND_FILE_PROMPT,
    STR_SCREEN_CAPTURE_PROMPT,

    /* Transfer strings */
    STR_TRANSFER_NEW,
    STR_TRANSFER_STARTED,
    STR_TRANSFER___,
    STR_TRANSFER_PAUSED,
    STR_TRANSFER_BROKEN,
    STR_TRANSFER_CANCELLED,
    STR_TRANSFER_COMPLETE,

    /* Cursor/Mouse strings */
    STR_CURSOR_CLICK_LEFT,
    STR_CURSOR_CLICK_RIGHT,

    /* Friend strings */
    STR_FRIEND_ALIAS,
    STR_FRIEND_PUBLIC_KEY,
    STR_FRIEND_AUTOACCEPT,
    STR_FRIEND_EXPORT_CHATLOG,
    STR_DELETE_FRIEND,

    /* Group chat strings */
    STR_GROUP_CREATE_TEXT,
    STR_GROUP_CREATE_VOICE,
    STR_GROUPCHAT_SETTINGS,
    STR_GROUP_NOTIFICATIONS,
    STR_GROUP_NOTIFICATIONS_ON,
    STR_GROUP_NOTIFICATIONS_MENTION,
    STR_GROUP_NOTIFICATIONS_OFF,
    STR_GROUP_TOPIC,

    /* TODO REPLACE or REMOVE */
    STR_GROUPCHAT_JOIN_AUDIO,

    /* A/V Call Strings */
    STR_CALL_START_AUDIO,
    STR_CALL_START_VIDEO,
    STR_CALL_DECLINE,
    STR_CALL_CANCELLED,
    STR_CALL_INVITED,
    STR_CALL_RINGING,
    STR_CALL_STARTED,
    STR_CALL_FRIEND_HAD_ENDED_CALL,
    STR_CALL_VIDEO_SHOW_INLINE,

    /* Settings strings */
    STR_PROFILE_BUTTON,
    STR_DEVICES_BUTTON,
    STR_USER_INTERFACE_BUTTON,
    STR_AUDIO_VIDEO_BUTTON,
    STR_ADVANCED_BUTTON,
    STR_NOTIFICATIONS_BUTTON,

    STR_AUTO_UPDATE,

    STR_PROFILE_SETTINGS,
    STR_PROFILE_PW_WARNING,
    STR_PROFILE_PW_NO_RECOVER,

    STR_DEVICES_ADD_NEW,
    STR_DEVICES_NUMBER,

    /* Old Strings, please create or use more discriptive */
    /* TODO REMOVE OLD ONES! */
    STR_ADDFRIENDS,
    STR_TOXID,
    STR_MESSAGE,
    STR_FILTER_ALL,
    STR_FILTER_ONLINE,
    STR_FILTER_CONTACT_TOGGLE,
    STR_ADD,
    STR_CREATEGROUPCHAT,
    STR_FRIENDREQUEST,
    STR_USERSETTINGS,
    STR_FRIEND_SETTINGS,
    STR_NAME,
    STR_STATUSMESSAGE,
    STR_PREVIEW,
    STR_AUDIOINPUTDEVICE,
    STR_AUDIOFILTERING,
    STR_AUDIOOUTPUTDEVICE,
    STR_VIDEOINPUTDEVICE,
    STR_PUSH_TO_TALK,

    // Status info
    STR_STATUS,
    STR_STATUS_ONLINE,
    STR_STATUS_AWAY,
    STR_STATUS_BUSY,

    // Settings Strings
    STR_SETTINGS_UI_MINI_ROSTER,
    STR_SETTINGS_UI_AUTO_HIDE_SIDEBAR,

    // Status strings
    STR_NOT_CONNECTED,
    STR_NOT_CONNECTED_SETTINGS,

    // Setting strings
    STR_OTHERSETTINGS,
    STR_UI,
    STR_USER_INTERFACE,
    STR_UTOX_SETTINGS,
    STR_NETWORK_SETTINGS,
    STR_AUDIO_VIDEO,
    STR_PROFILE_PASSWORD,
    STR_LOCK_UTOX,
    STR_LOCK,
    STR_SHOW_UI_PASSWORD,
    STR_SHOW_UI_PASSWORD_TOOLTIP,
    STR_HIDE_UI_PASSWORD,
    STR_HIDE_UI_PASSWORD_TOOLTIP,

    STR_DPI,
    STR_SAVELOCATION,
    STR_LANGUAGE,
    STR_NETWORK,
    STR_IPV6,
    STR_UDP,
    STR_PROXY,
    STR_PROXY_FORCE,
    STR_WARNING,
    STR_SAVE_CHAT_HISTORY,
    STR_AUDIONOTIFICATIONS,
    STR_RINGTONE,
    STR_IS_TYPING,
    STR_CLOSE_TO_TRAY,
    STR_START_IN_TRAY,
    STR_AUTO_STARTUP,
    STR_RANDOMIZE_NOSPAM,
    STR_REVERT_NOSPAM,
    STR_NOSPAM,
    STR_NOSPAM_WARNING,
    STR_SHOW_NOSPAM,
    STR_HIDE_NOSPAM,
    STR_BLOCK_FRIEND_REQUESTS,


    // Interact with texts / clipboard
    STR_COPY,
    STR_COPY_TOX_ID = STR_COPY,
    STR_COPYWITHOUTNAMES,
    STR_COPY_WITH_NAMES,
    STR_CUT,
    STR_PASTE,
    STR_DELETE,
    STR_SELECTALL,

    STR_REMOVE,
    STR_REMOVE_FRIEND,
    STR_REMOVE_GROUP,
    STR_LEAVE,
    STR_LEAVE_GROUP,
    STR_ACCEPT,
    STR_REQ_ACCEPT = STR_ACCEPT,
    STR_CTOPIC,
    STR_CHANGE_GROUP_TOPIC = STR_CTOPIC,
    STR_IGNORE,
    STR_REQ_DECLINE = STR_IGNORE,
    STR_SET_ALIAS,

    STR_ALIAS,

    STR_SENDMESSAGE,
    STR_SENDSCREENSHOT,

    STR_CLICKTOSAVE,
    STR_CLICKTOOPEN,
    STR_CANCELLED,

    STR_DPI_050,
    STR_DPI_060,
    STR_DPI_070,
    STR_DPI_080,
    STR_DPI_090,
    STR_DPI_100,
    STR_DPI_110,
    STR_DPI_120,
    STR_DPI_130,
    STR_DPI_140,
    STR_DPI_150,
    STR_DPI_160,
    STR_DPI_170,
    STR_DPI_180,
    STR_DPI_190,
    STR_DPI_200,
    STR_DPI_210,
    STR_DPI_220,
    STR_DPI_230,
    STR_DPI_240,
    STR_DPI_250,
    STR_DPI_260,
    STR_DPI_270,
    STR_DPI_280,
    STR_DPI_290,
    STR_DPI_300,
    STR_DPI_TINY,
    STR_DPI_NORMAL,
    STR_DPI_BIG,
    STR_DPI_LARGE,
    STR_DPI_HUGE,

    STR_CONTACT_SEARCH_ADD_HINT,

    STR_PROXY_DISABLED,
    STR_PROXY_FALLBACK,
    STR_PROXY_ALWAYS_USE,

    STR_PROXY_EDIT_HINT_IP,
    STR_PROXY_EDIT_HINT_PORT,

    STR_NO,
    STR_YES,
    STR_OFF,
    STR_ON,
    STR_SHOW,
    STR_HIDE,

    STR_VIDEO_IN_NONE,
    STR_AUDIO_IN_NONE = STR_VIDEO_IN_NONE,
    STR_VIDEO_IN_DESKTOP,

    STR_AUDIO_IN_DEFAULT_LOOPBACK,
    STR_AUDIO_IN_ANDROID,

    STR_DEFAULT_FRIEND_REQUEST_MESSAGE,

    STR_WINDOW_TITLE_VIDEO_PREVIEW,

    STR_MUTE,
    STR_UNMUTE,

    STR_SELECT_AVATAR_TITLE,
    STR_AVATAR_TOO_LARGE_MAX_SIZE_IS,
    STR_CANT_FIND_FILE_OR_EMPTY,

    STR_CLEAR_HISTORY,

    STR_THEME,
    STR_THEME_DEFAULT,
    STR_THEME_LIGHT,
    STR_THEME_DARK,
    STR_THEME_HIGHCONTRAST,
    STR_THEME_CUSTOM,
    STR_THEME_ZENBURN,
    STR_THEME_SOLARIZED_LIGHT,
    STR_THEME_SOLARIZED_DARK,

    STR_SEND_TYPING_NOTIFICATIONS,
    STR_STATUS_NOTIFICATIONS,
    STR_DELETE_MESSAGE,
    STR_KEEP,

    NUM_STRS // add strings before this line
} UTOX_I18N_STR;

STRING *ui_gettext(UTOX_LANG lang, UTOX_I18N_STR string_id);

UTOX_LANG ui_guess_lang_by_posix_locale(const char *locale, UTOX_LANG deflt);
UTOX_LANG ui_guess_lang_by_windows_lang_id(uint16_t lang_id, UTOX_LANG deflt);

#endif
