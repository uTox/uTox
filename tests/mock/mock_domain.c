#include "mock_domain.h"

#include "../../src/avatar.h"
#include "../../src/file_transfers.h"
#include "../../src/messages.h"
#include "../../src/native/clipboard.h"
#include "../../src/native/filesys.h"
#include "../../src/native/image.h"
#include "../../src/native/notify.h"
#include "../../src/native/os.h"
#include "../../src/self.h"
#include "../../src/theme.h"
#include "../../src/tox.h"
#include "../../src/ui.h"
#include "../../src/ui/button.h"
#include "../../src/ui/contextmenu.h"
#include "../../src/ui/draw.h"
#include "../../src/ui/dropdown.h"
#include "../../src/ui/edit.h"
#include "../../src/ui/scrollable.h"
#include "../../src/ui/switch.h"
#include "../../src/ui/text.h"
#include "../../src/ui/tooltip.h"
#include "../../src/utox.h"

#include "../../src/av/audio.h"
#include "../../src/av/utox_av.h"

#include "../../src/layout/background.h"
#include "../../src/layout/friend.h"
#include "../../src/layout/group.h"
#include "../../src/layout/settings.h"
#include "../../src/layout/sidebar.h"

#include <stdlib.h>
#include <string.h>

struct utox_self self;

FRIEND *mock_sel_friend;
GROUPCHAT *mock_sel_group;
ITEM_TYPE mock_sel_item_type = ITEM_NONE;

uint8_t mock_last_tox_msg;
uint32_t mock_last_tox_p1;
uint32_t mock_last_tox_p2;
void *mock_last_tox_data;
uint8_t mock_last_tox_blob[TOX_ADDRESS_SIZE];
size_t mock_last_tox_blob_len;

uint8_t mock_last_audio_msg;
uint32_t mock_last_audio_param1;

char mock_last_openurl[512];

uint32_t mock_tox_friend_count = 1;
uint32_t mock_tox_conference_count = 0;
char mock_tox_conference_title[64];
int mock_tox_conference_title_err;

UTOX_MSG mock_last_utox_msg;
uint16_t mock_last_utox_p1;
uint16_t mock_last_utox_p2;

uint32_t mock_tox_file_send_next;
uint32_t mock_last_file_kind;
uint64_t mock_last_file_size;
uint8_t mock_last_file_hash[TOX_HASH_LENGTH];
uint8_t mock_incoming_file_id[TOX_HASH_LENGTH];
uint8_t mock_last_filename[256];
size_t mock_last_filename_len;
TOX_ERR_FILE_SEND mock_tox_file_send_err;
bool mock_tox_file_control_ok = true;
TOX_FILE_CONTROL mock_last_file_control;
uint32_t mock_file_control_count;
uint32_t mock_file_send_chunk_count;
uint64_t mock_last_chunk_position;
size_t mock_last_chunk_length;

tox_file_recv_cb *mock_cb_file_recv;
tox_file_recv_control_cb *mock_cb_file_recv_control;
tox_file_recv_chunk_cb *mock_cb_file_recv_chunk;
tox_file_chunk_request_cb *mock_cb_file_chunk_request;

UTOX_TOX_THREAD_INIT tox_thread_init = UTOX_TOX_THREAD_INIT_SUCCESS;

bool have_focus = true;
double ui_scale = 10.0;
int font_small_lineheight = 12;
int font_msg_lineheight = 16;

SCROLLABLE scrollbar_friend;
SCROLLABLE scrollbar_group;
SCROLLABLE scrollbar_flist;
PANEL messages_friend = { .content_scroll = &scrollbar_friend };
PANEL messages_group = { .content_scroll = &scrollbar_group };
uint8_t cursor;

PANEL panel_chat, panel_overhead, panel_splash_page;
PANEL panel_friend, panel_add_friend, panel_friend_chat, panel_friend_video;
PANEL panel_friend_settings, panel_friend_request, panel_friend_confirm_deletion;
PANEL panel_group, panel_group_create, panel_group_chat, panel_group_video, panel_group_settings;
PANEL panel_settings_master, panel_profile_password, panel_profile_password_settings, panel_nospam_settings;

BUTTON button_settings, button_add_new_contact;
UISWITCH switch_friend_autoaccept_ft;
DROPDOWN dropdown_notify_groupchats;

static char edit_add_id_buf[256];
static char edit_add_msg_buf[256];
static char edit_chat_friend_buf[1024];
static char edit_chat_group_buf[1024];
static char edit_search_buf[256];
static char edit_pubkey_buf[TOX_PUBLIC_KEY_SIZE * 2 + 1];
static char edit_alias_buf[256];
static char edit_topic_buf[256];

EDIT edit_add_new_friend_id = {
    .data      = edit_add_id_buf,
    .data_size = sizeof edit_add_id_buf,
};
EDIT edit_add_new_friend_msg = {
    .data      = edit_add_msg_buf,
    .data_size = sizeof edit_add_msg_buf,
};
EDIT edit_chat_msg_friend = {
    .data      = edit_chat_friend_buf,
    .data_size = sizeof edit_chat_friend_buf,
};
EDIT edit_chat_msg_group = {
    .data      = edit_chat_group_buf,
    .data_size = sizeof edit_chat_group_buf,
};
EDIT edit_search = {
    .data      = edit_search_buf,
    .data_size = sizeof edit_search_buf,
};
EDIT edit_friend_pubkey = {
    .data      = edit_pubkey_buf,
    .data_size = sizeof edit_pubkey_buf,
};
EDIT edit_friend_alias = {
    .data      = edit_alias_buf,
    .data_size = sizeof edit_alias_buf,
};
EDIT edit_group_topic = {
    .data      = edit_topic_buf,
    .data_size = sizeof edit_topic_buf,
};

uint32_t COLOR_BKGRND_MAIN, COLOR_BKGRND_ALT, COLOR_BKGRND_AUX, COLOR_BKGRND_MENU;
uint32_t COLOR_BKGRND_MENU_HOVER, COLOR_BKGRND_MENU_ACTIVE, COLOR_BKGRND_LIST, COLOR_BKGRND_LIST_HOVER;
uint32_t COLOR_MAIN_TEXT, COLOR_MAIN_TEXT_CHAT, COLOR_MAIN_TEXT_SUBTEXT, COLOR_MAIN_TEXT_ACTION;
uint32_t COLOR_MAIN_TEXT_QUOTE, COLOR_MAIN_TEXT_RED, COLOR_MAIN_TEXT_URL, COLOR_MAIN_TEXT_HINT;
uint32_t COLOR_MSG_USER, COLOR_MSG_USER_PEND, COLOR_MSG_USER_ERROR, COLOR_MSG_CONTACT;
uint32_t COLOR_MENU_TEXT, COLOR_MENU_TEXT_SUBTEXT, COLOR_MENU_TEXT_ACTIVE;
uint32_t COLOR_LIST_TEXT, COLOR_LIST_TEXT_SUBTEXT;
uint32_t COLOR_AUX_EDGE_NORMAL, COLOR_AUX_EDGE_HOVER, COLOR_AUX_EDGE_ACTIVE, COLOR_AUX_TEXT;
uint32_t COLOR_AUX_ACTIVEOPTION_BKGRND, COLOR_AUX_ACTIVEOPTION_TEXT;
uint32_t COLOR_GROUP_SELF, COLOR_GROUP_PEER, COLOR_GROUP_AUDIO, COLOR_GROUP_MUTED;
uint32_t COLOR_SELECTION_BACKGROUND, COLOR_SELECTION_TEXT;
uint32_t COLOR_EDGE_NORMAL, COLOR_EDGE_ACTIVE, COLOR_EDGE_HOVER;
uint32_t COLOR_ACTIVEOPTION_BKGRND, COLOR_ACTIVEOPTION_TEXT;
uint32_t COLOR_STATUS_ONLINE, COLOR_STATUS_AWAY, COLOR_STATUS_BUSY;
uint32_t COLOR_BTN_SUCCESS_BKGRND, COLOR_BTN_SUCCESS_TEXT, COLOR_BTN_SUCCESS_BKGRND_HOVER, COLOR_BTN_SUCCESS_TEXT_HOVER;
uint32_t COLOR_BTN_WARNING_BKGRND, COLOR_BTN_WARNING_TEXT, COLOR_BTN_WARNING_BKGRND_HOVER, COLOR_BTN_WARNING_TEXT_HOVER;
uint32_t COLOR_BTN_DANGER_BACKGROUND, COLOR_BTN_DANGER_TEXT, COLOR_BTN_DANGER_BKGRND_HOVER, COLOR_BTN_DANGER_TEXT_HOVER;
uint32_t COLOR_BTN_DISABLED_BKGRND, COLOR_BTN_DISABLED_TEXT, COLOR_BTN_DISABLED_BKGRND_HOVER, COLOR_BTN_DISABLED_TRANSFER;
uint32_t COLOR_BTN_INPROGRESS_BKGRND, COLOR_BTN_INPROGRESS_TEXT, COLOR_BTN_DISABLED_FORGRND, COLOR_BTN_INPROGRESS_FORGRND;
uint32_t status_color[4];

void mock_domain_reset(void) {
    mock_sel_friend     = NULL;
    mock_sel_group      = NULL;
    mock_sel_item_type  = ITEM_NONE;
    mock_last_tox_msg   = 0;
    mock_last_tox_p1    = 0;
    mock_last_tox_p2    = 0;
    mock_last_tox_data  = NULL;
    mock_last_tox_blob_len = 0;
    memset(mock_last_tox_blob, 0, sizeof mock_last_tox_blob);
    mock_last_audio_msg     = 0;
    mock_last_audio_param1  = 0;
    mock_last_openurl[0]    = 0;
    have_focus              = true;
    tox_thread_init         = UTOX_TOX_THREAD_INIT_SUCCESS;
    mock_tox_friend_count         = 1;
    mock_tox_conference_count     = 0;
    mock_tox_conference_title[0]  = 0;
    mock_tox_conference_title_err = 0;
    mock_last_utox_msg = 0;
    mock_last_utox_p1  = 0;
    mock_last_utox_p2  = 0;
    mock_tox_file_send_next = 0;
    mock_last_file_kind = 0;
    mock_last_file_size = 0;
    memset(mock_last_file_hash, 0, sizeof mock_last_file_hash);
    memset(mock_incoming_file_id, 0xCD, sizeof mock_incoming_file_id);
    memset(mock_last_filename, 0, sizeof mock_last_filename);
    mock_last_filename_len = 0;
    mock_tox_file_send_err = TOX_ERR_FILE_SEND_OK;
    mock_tox_file_control_ok = true;
    mock_last_file_control = 0;
    mock_file_control_count = 0;
    mock_file_send_chunk_count = 0;
    mock_last_chunk_position = 0;
    mock_last_chunk_length = 0;
    mock_cb_file_recv = NULL;
    mock_cb_file_recv_control = NULL;
    mock_cb_file_recv_chunk = NULL;
    mock_cb_file_chunk_request = NULL;
    mock_time_set(0);
    memset(edit_add_id_buf, 0, sizeof edit_add_id_buf);
    memset(edit_add_msg_buf, 0, sizeof edit_add_msg_buf);
    edit_add_new_friend_id.length  = 0;
    edit_add_new_friend_msg.length = 0;
}

void postmessage_toxcore(uint8_t msg, uint32_t param1, uint32_t param2, void *data) {
    mock_last_tox_msg  = msg;
    mock_last_tox_p1   = param1;
    mock_last_tox_p2   = param2;
    mock_last_tox_data = data;
    mock_last_tox_blob_len = 0;
    if (data && msg == TOX_FRIEND_NEW_NO_REQ && param1 > 0 && param1 <= sizeof mock_last_tox_blob) {
        memcpy(mock_last_tox_blob, data, param1);
        mock_last_tox_blob_len = param1;
    }

    switch (msg) {
        case TOX_FRIEND_NEW:
        case TOX_FRIEND_NEW_NO_REQ:
        case TOX_FRIEND_NEW_DEVICE:
        case TOX_GROUP_SET_TOPIC:
            free(data);
            mock_last_tox_data = NULL;
            break;
        case TOX_FILE_SEND_NEW_INLINE:
            free(data);
            mock_last_tox_data = NULL;
            break;
        default:
            break;
    }
}

void postmessage_utox(UTOX_MSG msg, uint16_t param1, uint16_t param2, void *data) {
    mock_last_utox_msg = msg;
    mock_last_utox_p1  = param1;
    mock_last_utox_p2  = param2;

    switch (msg) {
        case FILE_STATUS_UPDATE:
        case FILE_INCOMING_NEW:
        case FILE_SEND_NEW:
            free(data);
            break;
        case FILE_INCOMING_NEW_INLINE: {
            if (data) {
                NATIVE_IMAGE *img = NULL;
                memcpy(&img, (uint8_t *)data + sizeof(uint16_t) * 2, sizeof(NATIVE_IMAGE *));
                image_free(img);
                free(data);
            }
            break;
        }
        case FILE_STATUS_UPDATE_DATA: {
            FILE_TRANSFER *ft = data;
            if (ft) {
                if (ft->incoming && ft->in_memory) {
                    free(ft->via.memory);
                    ft->via.memory = NULL;
                } else if (ft->incoming && ft->avatar) {
                    free(ft->via.avatar);
                    ft->via.avatar = NULL;
                }
                ft->decon_wait = false;
            }
            break;
        }
        default:
            break;
    }
}

void postmessage_audio(uint8_t msg, uint32_t param1, uint32_t param2, void *data) {
    mock_last_audio_msg    = msg;
    mock_last_audio_param1 = param1;
    (void)param2;
    (void)data;
}

void notify(char *title, uint16_t title_length, const char *msg, uint16_t msg_length, void *object, bool is_group) {
    (void)title;
    (void)title_length;
    (void)msg;
    (void)msg_length;
    (void)object;
    (void)is_group;
}

bool avatar_init(char hexid[TOX_PUBLIC_KEY_SIZE * 2], AVATAR *avatar) {
    (void)hexid;
    if (avatar) {
        memset(avatar, 0, sizeof(*avatar));
    }
    return false;
}

void maybe_i18nal_string_set_plain(MAYBE_I18NAL_STRING *s, char *str, uint16_t length) {
    if (!s) {
        return;
    }
    s->plain.str    = str;
    s->plain.length = length;
}

STRING *ui_gettext(UTOX_LANG lang, UTOX_I18N_STR string_id) {
    (void)lang;
    (void)string_id;
    static STRING s = { .str = "%.*s %s", .length = 6 };
    return &s;
}

NATIVE_IMAGE *utox_image_to_native(const UTOX_IMAGE data, size_t size, uint16_t *w, uint16_t *h, bool keep_alpha) {
    (void)data;
    (void)size;
    (void)keep_alpha;
    if (w) {
        *w = 1;
    }
    if (h) {
        *h = 1;
    }
    return (NATIVE_IMAGE *)calloc(1, 1);
}

void image_free(NATIVE_IMAGE *image) {
    free(image);
}

#ifndef NATIVE_IMAGE_IS_VALID
/* Cocoa exposes this as a function; Win/X11 use a macro. */
int NATIVE_IMAGE_IS_VALID(NATIVE_IMAGE *img) {
    return img != NULL;
}
#endif

void image_set_filter(NATIVE_IMAGE *image, uint8_t filter) {
    (void)image;
    (void)filter;
}

void image_set_scale(NATIVE_IMAGE *image, double scale) {
    (void)image;
    (void)scale;
}

void draw_image(const NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t imgx, uint32_t imgy) {
    (void)image;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)imgx;
    (void)imgy;
}

void drawtext(int x, int y, const char *str, uint16_t length) {
    (void)x;
    (void)y;
    (void)str;
    (void)length;
}

void drawtextwidth_right(int x, int width, int y, const char *str, uint16_t length) {
    (void)x;
    (void)width;
    (void)y;
    (void)str;
    (void)length;
}

void drawtextrange(int x, int x2, int y, const char *str, uint16_t length) {
    (void)x;
    (void)x2;
    (void)y;
    (void)str;
    (void)length;
}

void draw_rect_fill(int x, int y, int width, int height, uint32_t color) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
}

void drawrect(int x, int y, int right, int bottom, uint32_t color) {
    (void)x;
    (void)y;
    (void)right;
    (void)bottom;
    (void)color;
}

void force_redraw(void) {}

void reset_settings_controls(void) {}

void tooltip_draw(void) {}

bool tooltip_mdown(void) {
    return false;
}

bool tooltip_mup(void) {
    return false;
}

void drawalpha(int bm, int x, int y, int width, int height, uint32_t color) {
    (void)bm;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
}

void setfont(int id) {
    (void)id;
}

uint32_t setcolor(uint32_t color) {
    return color;
}

int textwidth(const char *str, uint16_t length) {
    (void)str;
    return length * 6;
}

int text_height(int right, uint16_t lineheight, char *str, uint16_t length) {
    (void)right;
    (void)str;
    (void)length;
    return lineheight ? lineheight : 12;
}

uint16_t hittextmultiline(int mx, int right, int my, int height, uint16_t lineheight, char *str, uint16_t length,
                          bool multiline) {
    (void)mx;
    (void)right;
    (void)my;
    (void)height;
    (void)lineheight;
    (void)str;
    (void)multiline;
    return length ? 1 : 0;
}

int utox_draw_text_multiline_within_box(int x, int y, int right, int top, int bottom, uint16_t lineheight,
                                        const char *data, uint16_t length, uint16_t h, uint16_t hlen, uint16_t mark,
                                        uint16_t marklen, bool multiline) {
    (void)x;
    (void)y;
    (void)right;
    (void)top;
    (void)bottom;
    (void)lineheight;
    (void)data;
    (void)length;
    (void)h;
    (void)hlen;
    (void)mark;
    (void)marklen;
    (void)multiline;
    return lineheight;
}

int scroll_gety(SCROLLABLE *s, int height) {
    (void)height;
    if (!s) {
        return 0;
    }
    return (int)(s->d * (double)(s->content_height > height ? s->content_height - height : 0));
}

void edit_setfocus(EDIT *edit) {
    (void)edit;
}

void edit_resetfocus(void) {}

void edit_setstr(EDIT *edit, char *str, uint16_t length) {
    if (!edit || !edit->data || !edit->data_size) {
        return;
    }
    if (length >= edit->data_size) {
        length = (uint16_t)(edit->data_size - 1);
    }
    if (str && length) {
        memcpy(edit->data, str, length);
    }
    edit->data[length] = 0;
    edit->length = length;
}

void edit_paste(char *data, int length, bool select) {
    (void)data;
    (void)length;
    (void)select;
}

void redraw(void) {}

void draw_avatar_image(NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t targetwidth,
                       uint32_t targetheight) {
    (void)image;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)targetwidth;
    (void)targetheight;
}

void id_to_string(char *dest, uint8_t *src) {
    static const char hex[] = "0123456789ABCDEF";
    if (!dest) {
        return;
    }
    if (!src) {
        dest[0] = 0;
        return;
    }
    for (size_t i = 0; i < TOX_ADDRESS_SIZE; i++) {
        dest[i * 2]     = hex[(src[i] >> 4) & 0xF];
        dest[i * 2 + 1] = hex[src[i] & 0xF];
    }
    dest[TOX_ADDRESS_SIZE * 2] = 0;
}

void contextmenu_new(uint8_t count, UTOX_I18N_STR *menu_string_ids, void (*onselect)(uint8_t)) {
    (void)menu_string_ids;
    if (onselect && count) {
        onselect(0);
        if (count > 1) {
            onselect(1);
        }
    }
}

void copy(int value) {
    (void)value;
}

void setselection(char *data, uint16_t length) {
    (void)data;
    (void)length;
}

void openurl(char *str) {
    if (!str) {
        mock_last_openurl[0] = 0;
        return;
    }
    strncpy(mock_last_openurl, str, sizeof mock_last_openurl - 1);
    mock_last_openurl[sizeof mock_last_openurl - 1] = 0;
}

void file_save_inline_image_png(MSG_HEADER *msg) {
    (void)msg;
}

void native_select_dir_ft(uint32_t fid, uint32_t num, FILE_TRANSFER *file) {
    (void)fid;
    (void)num;
    (void)file;
}

void group_av_peer_add(GROUPCHAT *g, int peernumber) {
    (void)g;
    (void)peernumber;
}

void native_export_chatlog_init(uint32_t friend_number) {
    (void)friend_number;
}

bool tox_friend_get_public_key(const Tox *tox, uint32_t friend_number, uint8_t public_key[TOX_PUBLIC_KEY_SIZE],
                               TOX_ERR_FRIEND_GET_PUBLIC_KEY *error) {
    (void)tox;
    (void)friend_number;
    if (error) {
        *error = TOX_ERR_FRIEND_GET_PUBLIC_KEY_OK;
    }
    if (public_key) {
        memset(public_key, 0xAB, TOX_PUBLIC_KEY_SIZE);
        public_key[0] = (uint8_t)friend_number;
    }
    return true;
}

size_t tox_friend_get_name_size(const Tox *tox, uint32_t friend_number, TOX_ERR_FRIEND_QUERY *error) {
    (void)tox;
    (void)friend_number;
    if (error) {
        *error = TOX_ERR_FRIEND_QUERY_OK;
    }
    return 5;
}

bool tox_friend_get_name(const Tox *tox, uint32_t friend_number, uint8_t name[], TOX_ERR_FRIEND_QUERY *error) {
    (void)tox;
    (void)friend_number;
    if (error) {
        *error = TOX_ERR_FRIEND_QUERY_OK;
    }
    memcpy(name, "Alice", 5);
    return true;
}

size_t tox_friend_get_status_message_size(const Tox *tox, uint32_t friend_number, TOX_ERR_FRIEND_QUERY *error) {
    (void)tox;
    (void)friend_number;
    if (error) {
        *error = TOX_ERR_FRIEND_QUERY_OK;
    }
    return 2;
}

bool tox_friend_get_status_message(const Tox *tox, uint32_t friend_number, uint8_t status_message[],
                                   TOX_ERR_FRIEND_QUERY *error) {
    (void)tox;
    (void)friend_number;
    if (error) {
        *error = TOX_ERR_FRIEND_QUERY_OK;
    }
    memcpy(status_message, "hi", 2);
    return true;
}

TOX_USER_STATUS tox_friend_get_status(const Tox *tox, uint32_t friend_number, TOX_ERR_FRIEND_QUERY *error) {
    (void)tox;
    (void)friend_number;
    if (error) {
        *error = TOX_ERR_FRIEND_QUERY_OK;
    }
    return TOX_USER_STATUS_NONE;
}

TOX_CONNECTION tox_friend_get_connection_status(const Tox *tox, uint32_t friend_number, TOX_ERR_FRIEND_QUERY *error) {
    (void)tox;
    (void)friend_number;
    if (error) {
        *error = TOX_ERR_FRIEND_QUERY_OK;
    }
    return TOX_CONNECTION_UDP;
}

size_t tox_self_get_friend_list_size(const Tox *tox) {
    (void)tox;
    return mock_tox_friend_count;
}

size_t tox_conference_get_chatlist_size(const Tox *tox) {
    (void)tox;
    return mock_tox_conference_count;
}

void tox_conference_get_chatlist(const Tox *tox, uint32_t chatlist[]) {
    (void)tox;
    if (!chatlist) {
        return;
    }
    for (uint32_t i = 0; i < mock_tox_conference_count; i++) {
        chatlist[i] = i;
    }
}

size_t tox_conference_get_title_size(const Tox *tox, uint32_t conference_number, TOX_ERR_CONFERENCE_TITLE *error) {
    (void)tox;
    (void)conference_number;
    if (mock_tox_conference_title_err) {
        if (error) {
            *error = (TOX_ERR_CONFERENCE_TITLE)mock_tox_conference_title_err;
        }
        return 0;
    }
    if (error) {
        *error = TOX_ERR_CONFERENCE_TITLE_OK;
    }
    return strlen(mock_tox_conference_title);
}

bool tox_conference_get_title(const Tox *tox, uint32_t conference_number, uint8_t title[],
                              TOX_ERR_CONFERENCE_TITLE *error) {
    (void)tox;
    (void)conference_number;
    if (error) {
        *error = TOX_ERR_CONFERENCE_TITLE_OK;
    }
    if (title && mock_tox_conference_title[0]) {
        memcpy(title, mock_tox_conference_title, strlen(mock_tox_conference_title));
    }
    return true;
}

void tox_callback_file_recv(Tox *tox, tox_file_recv_cb *callback) {
    (void)tox;
    mock_cb_file_recv = callback;
}

void tox_callback_file_recv_control(Tox *tox, tox_file_recv_control_cb *callback) {
    (void)tox;
    mock_cb_file_recv_control = callback;
}

void tox_callback_file_recv_chunk(Tox *tox, tox_file_recv_chunk_cb *callback) {
    (void)tox;
    mock_cb_file_recv_chunk = callback;
}

void tox_callback_file_chunk_request(Tox *tox, tox_file_chunk_request_cb *callback) {
    (void)tox;
    mock_cb_file_chunk_request = callback;
}

bool tox_hash(Tox_Hash hash, const uint8_t data[], size_t length) {
    if (!hash) {
        return false;
    }
    memset(hash, 0, TOX_HASH_LENGTH);
    if (data && length) {
        hash[0] = data[0];
        hash[1] = (uint8_t)length;
    }
    return true;
}

uint32_t tox_file_send(Tox *tox, uint32_t friend_number, uint32_t kind, uint64_t file_size,
                       const Tox_File_Id file_id, const uint8_t filename[], size_t filename_length,
                       TOX_ERR_FILE_SEND *error) {
    (void)tox;
    (void)friend_number;
    mock_last_file_kind = kind;
    mock_last_file_size = file_size;
    memset(mock_last_file_hash, 0, sizeof mock_last_file_hash);
    if (file_id) {
        memcpy(mock_last_file_hash, file_id, TOX_HASH_LENGTH);
    }
    mock_last_filename_len = 0;
    memset(mock_last_filename, 0, sizeof mock_last_filename);
    if (filename && filename_length) {
        mock_last_filename_len = filename_length < sizeof mock_last_filename ? filename_length
                                                                            : sizeof mock_last_filename - 1;
        memcpy(mock_last_filename, filename, mock_last_filename_len);
    }

    if (mock_tox_file_send_err != TOX_ERR_FILE_SEND_OK) {
        if (error) {
            *error = mock_tox_file_send_err;
        }
        return UINT32_MAX;
    }

    if (error) {
        *error = TOX_ERR_FILE_SEND_OK;
    }
    return mock_tox_file_send_next++;
}

bool tox_file_control(Tox *tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control,
                      TOX_ERR_FILE_CONTROL *error) {
    (void)tox;
    (void)friend_number;
    (void)file_number;
    mock_last_file_control = control;
    mock_file_control_count++;
    if (!mock_tox_file_control_ok) {
        if (error) {
            *error = TOX_ERR_FILE_CONTROL_FRIEND_NOT_CONNECTED;
        }
        return false;
    }
    if (error) {
        *error = TOX_ERR_FILE_CONTROL_OK;
    }
    return true;
}

bool tox_file_send_chunk(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position,
                         const uint8_t data[], size_t length, TOX_ERR_FILE_SEND_CHUNK *error) {
    (void)tox;
    (void)friend_number;
    (void)file_number;
    (void)data;
    mock_file_send_chunk_count++;
    mock_last_chunk_position = position;
    mock_last_chunk_length   = length;
    if (error) {
        *error = TOX_ERR_FILE_SEND_CHUNK_OK;
    }
    return true;
}

bool tox_file_get_file_id(const Tox *tox, uint32_t friend_number, uint32_t file_number, Tox_File_Id file_id,
                          TOX_ERR_FILE_GET *error) {
    (void)tox;
    (void)friend_number;
    (void)file_number;
    if (error) {
        *error = TOX_ERR_FILE_GET_OK;
    }
    if (file_id) {
        memcpy(file_id, mock_incoming_file_id, TOX_HASH_LENGTH);
    }
    return true;
}

bool tox_file_seek(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position,
                   TOX_ERR_FILE_SEEK *error) {
    (void)tox;
    (void)friend_number;
    (void)file_number;
    (void)position;
    if (error) {
        *error = TOX_ERR_FILE_SEEK_OK;
    }
    return true;
}
