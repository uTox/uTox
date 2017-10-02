#include "messages.h"

#include "chatlog.h"
#include "file_transfers.h"
#include "filesys.h"
#include "flist.h"
#include "friend.h"
#include "groups.h"
#include "debug.h"
#include "macros.h"
#include "self.h"
#include "settings.h"
#include "text.h"
#include "theme.h"
#include "tox.h"
#include "utox.h"

#include "ui/contextmenu.h"
#include "ui/draw.h"
#include "ui/scrollable.h"
#include "ui/svg.h"
#include "ui/text.h"

#include "layout/friend.h"
#include "layout/group.h"

#include "native/clipboard.h"
// TODO including native .h files should never be needed, refactor filesys.h to provide necessary API
#include "native/filesys.h"
#include "native/image.h"
#include "native/keyboard.h"
#include "native/os.h"

#include <stdlib.h>
#include <string.h>

#define UTOX_MAX_BACKLOG_MESSAGES 256

/** Appends a messages from self or friend to the message list;
 * will realloc or trim messages as needed;
 *
 * also handles auto scrolling selections with messages
 *
 * accepts: MESSAGES *pointer, MESSAGE *pointer, MSG_DATA *pointer
 */

static int get_time_width() {
    return SCALE(settings.use_long_time_msg ? TIME_WIDTH_LONG : TIME_WIDTH);
}

static int msgheight(MSG_HEADER *msg, int width) {
    switch (msg->msg_type) {
        case MSG_TYPE_NULL: {
            LOG_ERR("Messages", "Invalid message type in msgheight.");
            return 0;
        }

        case MSG_TYPE_TEXT:
        case MSG_TYPE_ACTION_TEXT:
        case MSG_TYPE_NOTICE:
        case MSG_TYPE_NOTICE_DAY_CHANGE: {
            int  theight = text_height(abs(width - MESSAGES_X - get_time_width()), font_small_lineheight,
                                       msg->via.txt.msg, msg->via.txt.length);
            return (theight == 0) ? 0 : theight + MESSAGES_SPACING;
        }

        case MSG_TYPE_IMAGE: {
            uint32_t maxwidth = width - MESSAGES_X - get_time_width();
            if (msg->via.img.zoom || msg->via.img.w <= maxwidth) {
                return msg->via.img.h + MESSAGES_SPACING;
            }

            return msg->via.img.h * maxwidth / msg->via.img.w + MESSAGES_SPACING;
        }

        case MSG_TYPE_FILE: {
            return FILE_TRANSFER_BOX_HEIGHT + MESSAGES_SPACING;
        }
    }

    return 0;
}

static int msgheight_group(MSG_HEADER *msg, int width) {
    switch (msg->msg_type) {
        case MSG_TYPE_TEXT:
        case MSG_TYPE_ACTION_TEXT:
        case MSG_TYPE_NOTICE:
        case MSG_TYPE_NOTICE_DAY_CHANGE: {
            int theight = text_height(abs(width - MESSAGES_X - get_time_width()), font_small_lineheight,
                                      msg->via.grp.msg, msg->via.grp.length);
            return (theight == 0) ? 0 : theight + MESSAGES_SPACING;
        }

        default: {
            LOG_TRACE("Messages", "Error, can't set this group message height" );
        }
    }

    return 0;
}

static int message_setheight(MESSAGES *m, MSG_HEADER *msg) {
    if (m->width == 0) {
        return 0;
    }

    setfont(FONT_TEXT);

    if (m->is_groupchat) {
        msg->height    = msgheight_group(msg, m->width);
    } else {
        msg->height = msgheight(msg, m->width);
    }

    return msg->height;
}

static void message_updateheight(MESSAGES *m, MSG_HEADER *msg) {
    if (m->width == 0) {
        return;
    }

    setfont(FONT_TEXT);

    m->height   -= msg->height;
    msg->height  = message_setheight(m, msg);
    m->height   += msg->height;
}

static uint32_t message_add(MESSAGES *m, MSG_HEADER *msg) {
    pthread_mutex_lock(&messages_lock);

    if (m->number < UTOX_MAX_BACKLOG_MESSAGES) {
        if (!m->data || m->extra <= 0) {
            if (m->data) {
                m->data = realloc(m->data, (m->number + 10) * sizeof(void *));
                m->extra += 10;
            } else {
                m->data = calloc(20, sizeof(void *));
                m->extra = 20;
            }

            if (!m->data) {
                LOG_FATAL_ERR(EXIT_MALLOC, "Messages", "\n\n\nFATAL ERROR TRYING TO REALLOC FOR MESSAGES.\nTHIS IS A BUG, PLEASE REPORT!\n\n\n");
            }
        }
        m->data[m->number++] = msg;
        m->extra--;
    } else {
        m->height -= m->data[0]->height;
        message_free(m->data[0]);
        memmove(m->data, m->data + 1, (UTOX_MAX_BACKLOG_MESSAGES - 1) * sizeof(MSG_HEADER *));
        m->data[UTOX_MAX_BACKLOG_MESSAGES - 1] = msg;

        // Scroll selection up so that it stays over the same messages.
        if (m->sel_start_msg != UINT32_MAX) {
            if (0 < m->sel_start_msg) {
                m->sel_start_msg--;
            } else {
                m->sel_start_position = 0;
            }
        }

        if (m->sel_end_msg != UINT32_MAX) {
            if (0 < m->sel_end_msg) {
                m->sel_end_msg--;
            } else {
                m->sel_end_position = 0;
            }
        }

        if (m->cursor_down_msg != UINT32_MAX) {
            if (0 < m->cursor_down_msg) {
                m->cursor_down_msg--;
            } else {
                m->cursor_down_position = 0;
            }
        }
        if (m->cursor_over_msg != UINT32_MAX) {
            if (0 < m->cursor_over_msg) {
                m->cursor_over_msg--;
            } else {
                m->cursor_over_position = 0;
            }
        }
    }

    message_updateheight(m, msg);

    if (m->is_groupchat) {
        GROUPCHAT *g = flist_get_groupchat();

        if (g && g == get_group(m->id)) {
            m->panel.content_scroll->content_height = m->height;
        }
    } else if (flist_get_friend() && flist_get_friend()->number == get_friend(m->id)->number) {
        m->panel.content_scroll->content_height = m->height;
    }

    pthread_mutex_unlock(&messages_lock);
    return m->number;
}

static bool msg_add_day_notice(MESSAGES *m, time_t last, time_t next) {
    /* The tm struct is shared, we have to do it this way */
    int ltime_year = 0, ltime_mon = 0, ltime_day = 0;

    struct tm *msg_time = localtime(&last);

    ltime_year = msg_time->tm_year;
    ltime_mon  = msg_time->tm_mon;
    ltime_day  = msg_time->tm_mday;
    msg_time   = localtime(&next);

    if (ltime_year < msg_time->tm_year
        || (ltime_year == msg_time->tm_year && ltime_mon < msg_time->tm_mon)
        || (ltime_year == msg_time->tm_year && ltime_mon == msg_time->tm_mon && ltime_day < msg_time->tm_mday))
    {
        MSG_HEADER *msg = calloc(1, sizeof(MSG_HEADER));
        if (!msg) {
            LOG_ERR("Messages", "Couldn't allocate memory for day notice.");
            return false;
        }

        time(&msg->time);
        msg->our_msg       = 0;
        msg->msg_type      = MSG_TYPE_NOTICE_DAY_CHANGE;

        msg->via.notice_day.msg    = calloc(1, 256);
        msg->via.notice_day.length = strftime((char *)msg->via.notice_day.msg, 256, "Day has changed to %A %B %d %Y", msg_time);

        message_add(m, msg);
        return true;
    }
    return false;
}

/* TODO leaving this here is a little hacky, but it was the fastest way
 * without considering if I should expose messages_add */
uint32_t message_add_group(MESSAGES *m, MSG_HEADER *msg) {
    return message_add(m, msg);
}

uint32_t message_add_type_text(MESSAGES *m, bool auth, const char *msgtxt, uint16_t length, bool log, bool send) {
    MSG_HEADER *msg = calloc(1, sizeof(MSG_HEADER));
    time(&msg->time);
    msg->our_msg  = auth;
    msg->msg_type = MSG_TYPE_TEXT;

    msg->via.txt.msg    = calloc(1, length);
    msg->via.txt.length = length;

    FRIEND *f = get_friend(m->id);

    if (auth) {
        msg->via.txt.author_length = self.name_length;
        if (!send) {
            msg->receipt      = 0;
            msg->receipt_time = 1;
        }
    } else {
        msg->via.txt.author_length = f->name_length;
    }

    memcpy(msg->via.txt.msg, msgtxt, length);

    if (m->data && m->number) {
        MSG_HEADER *day_msg = m->data[m->number ? m->number - 1 : 0];
        msg_add_day_notice(m, day_msg->time, msg->time);
    }

    if (log) {
        message_log_to_disk(m, msg);
    }

    if (auth && send) {
        postmessage_toxcore(TOX_SEND_MESSAGE, m->id, length, msg);
    }

    return message_add(m, msg);
}

uint32_t message_add_type_action(MESSAGES *m, bool auth, const char *msgtxt, uint16_t length, bool log, bool send) {
    MSG_HEADER *msg = calloc(1, sizeof(MSG_HEADER));

    time(&msg->time);
    msg->our_msg  = auth;
    msg->msg_type = MSG_TYPE_ACTION_TEXT;

    msg->via.action.msg = calloc(1, length);
    msg->via.action.length = length;

    FRIEND *f = get_friend(m->id);

    if (auth) {
        msg->via.txt.author_length = self.name_length;
        if (!send) {
            msg->receipt      = 0;
            msg->receipt_time = 1;
        }
    } else {
        msg->via.txt.author_length = f->name_length;
    }

    memcpy(msg->via.action.msg, msgtxt, length);

    if (log) {
        message_log_to_disk(m, msg);
    }

    if (auth && send) {
        postmessage_toxcore(TOX_SEND_ACTION, f->number, length, msg);
    }

    return message_add(m, msg);
}

uint32_t message_add_type_notice(MESSAGES *m, const char *msgtxt, uint16_t length, bool log) {
    MSG_HEADER *msg = calloc(1, sizeof(MSG_HEADER));
    if (!msg) {
        LOG_ERR("Messages", "Couldn't allocate memory for notice.");
        return UINT32_MAX;
    }

    time(&msg->time);
    msg->our_msg       = 0;
    msg->msg_type      = MSG_TYPE_NOTICE;
    msg->via.txt.author_length = self.name_length;
    msg->receipt_time  = time(NULL);

    msg->via.notice.length = length;
    msg->via.notice.msg = calloc(1, length);
    memcpy(msg->via.notice.msg, msgtxt, length);

    if (log) {
        message_log_to_disk(m, msg);
    }

    return message_add(m, msg);
}

uint32_t message_add_type_image(MESSAGES *m, bool auth, NATIVE_IMAGE *img, uint16_t width, uint16_t height, bool UNUSED(log)) {
    if (!NATIVE_IMAGE_IS_VALID(img)) {
        return 0;
    }

    MSG_HEADER *msg = calloc(1, sizeof(MSG_HEADER));
    time(&msg->time);
    msg->our_msg  = auth;
    msg->msg_type = MSG_TYPE_IMAGE;

    msg->via.img.w        = width;
    msg->via.img.h        = height;
    msg->via.img.zoom     = 0;
    msg->via.img.image    = img;
    msg->via.img.position = 0.0;

    return message_add(m, msg);
}

/* TODO FIX THIS SECTION TO MATCH ABOVE! */
/* Called by new file transfer to add a new message to the msg list */
MSG_HEADER *message_add_type_file(MESSAGES *m, uint32_t file_number, bool incoming, bool image, uint8_t status,
                                const uint8_t *name, size_t name_size, size_t target_size, size_t current_size)
{
    MSG_HEADER *msg = calloc(1, sizeof(MSG_HEADER));

    time(&msg->time);
    msg->our_msg     = !incoming;
    msg->msg_type    = MSG_TYPE_FILE;

    msg->via.ft.file_status = status;

    msg->via.ft.file_number = file_number;

    msg->via.ft.size       = target_size;
    msg->via.ft.progress   = current_size;
    msg->via.ft.speed      = 0;
    msg->via.ft.inline_png = image;

    msg->via.ft.name_length = name_size;
    msg->via.ft.name = calloc(1, name_size + 1);
    memcpy(msg->via.ft.name, name, msg->via.ft.name_length);

    if (image) {
        msg->via.ft.path = NULL;
    } else { // It's a file
        msg->via.ft.path = calloc(1, UTOX_FILE_NAME_LENGTH);
    }

    message_add(m, msg);

    return msg;
}

bool message_log_to_disk(MESSAGES *m, MSG_HEADER *msg) {
    if (m->is_groupchat) {
        /* We don't support logging groupchats yet */
        return false;
    }

    if (!settings.logging_enabled) {
        return false;
    }

    FRIEND *f = get_friend(m->id);

    if (!f) {
        LOG_ERR("Messages", "Could not get friend with number: %u", m->id);
        return false;
    }

    if (f->skip_msg_logging) {
        return false;
    }

    LOG_FILE_MSG_HEADER header;
    memset(&header, 0, sizeof(header));

    switch (msg->msg_type) {
        case MSG_TYPE_TEXT:
        case MSG_TYPE_ACTION_TEXT:
        case MSG_TYPE_NOTICE: {
            size_t author_length;
            char   *author;
            if (msg->our_msg) {
                author_length = self.name_length;
                author        = self.name;
            } else {
                author_length = f->name_length;
                author        = f->name;
            }

            header.log_version   = LOGFILE_SAVE_VERSION;
            header.time          = msg->time;
            header.author_length = author_length;
            header.msg_length    = msg->via.txt.length;
            header.author        = msg->our_msg;
            header.receipt       = !!msg->receipt_time; // bool only
            header.msg_type      = msg->msg_type;

            size_t length = sizeof(header) + msg->via.txt.length + author_length + 1; /* extra \n char*/

            uint8_t *data = calloc(1, length);
            if (!data) {
                LOG_FATAL_ERR(EXIT_MALLOC, "Messages", "Can't calloc for chat logging data. size:%lu", length);
            }
            memcpy(data, &header, sizeof(header));
            memcpy(data + sizeof(header), author, author_length);
            memcpy(data + sizeof(header) + author_length, msg->via.txt.msg, msg->via.txt.length);
            strcpy2(data + length - 1, "\n");

            msg->disk_offset = utox_save_chatlog(f->id_str, data, length);

            free(data);
            return true;
        }
        default: {
            LOG_NOTE("Messages", "uTox Logging:\tUnsupported message type %i", msg->msg_type);
        }
    }
    return false;
}

bool messages_read_from_log(uint32_t friend_number) {
    size_t    actual_count = 0;

    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_ERR("Messages", "Could not get friend with number: %u", friend_number);
        return false;
    }

    MSG_HEADER **data = utox_load_chatlog(f->id_str, &actual_count, UTOX_MAX_BACKLOG_MESSAGES, 0);

    time_t last = 0;

    if (data) {
        MSG_HEADER **p = data;
        MSG_HEADER *msg;
        while (actual_count--) {
            msg = *p++;
            if (msg) {
                if (msg_add_day_notice(&f->msg, last, msg->time)) {
                    last = msg->time;
                }

                message_add(&f->msg, msg);
            }
        }
        free(data);
        return true;
    } else if (actual_count > 0) {
        LOG_ERR("Messages", "uTox Logging:\tFound chat log entries, but couldn't get any data. This is a problem.");
    }

    return false;
}

void messages_send_from_queue(MESSAGES *m, uint32_t friend_number) {
    uint32_t start    = m->number;
    uint8_t  seek_num = 3; /* this magic number is the number of messages we'll skip looking for the first unsent */

    pthread_mutex_lock(&messages_lock);

    int queue_count = 0;
    /* seek back to find first queued message
     * I hate this nest too, but it's readable */
    while (start) {
        --start;

        if (++queue_count > 25) {
            break;
        }

        if (m->data[start]) {
            MSG_HEADER *msg = m->data[start];
            if (msg->msg_type == MSG_TYPE_TEXT || msg->msg_type == MSG_TYPE_ACTION_TEXT) {
                if (msg->our_msg) {
                    if (msg->receipt_time) {
                        if (!seek_num--) {
                            break;
                        }
                    }
                }
            }
        }
    }

    int sent_count = 0;
    /* start sending messages, hopefully in order */
    while (start < m->number && sent_count <= 25) {
        if (m->data[start]) {
            MSG_HEADER *msg = m->data[start];
            if (msg->msg_type == MSG_TYPE_TEXT || msg->msg_type == MSG_TYPE_ACTION_TEXT) {
                if (msg->our_msg && !msg->receipt_time) {
                    postmessage_toxcore((msg->msg_type == MSG_TYPE_TEXT ? TOX_SEND_MESSAGE : TOX_SEND_ACTION),
                                        friend_number, msg->via.txt.length, msg);
                    ++sent_count;
                }
            }
        }
        ++start;
    }
    pthread_mutex_unlock(&messages_lock);
}

void messages_clear_receipt(MESSAGES *m, uint32_t receipt_number) {
    pthread_mutex_lock(&messages_lock);

    uint32_t start = m->number;

    while (start--) {
        if (m->data[start]) {
            MSG_HEADER *msg = m->data[start];
            if (msg->msg_type == MSG_TYPE_TEXT || msg->msg_type == MSG_TYPE_ACTION_TEXT) {
                if (msg->receipt == receipt_number) {
                    msg->receipt = -1;
                    time(&msg->receipt_time);

                    LOG_FILE_MSG_HEADER header;
                    memset(&header, 0, sizeof(header));

                    header.log_version   = LOGFILE_SAVE_VERSION;
                    header.time          = msg->time;
                    header.author_length = msg->via.txt.author_length;
                    header.msg_length    = msg->via.txt.length;
                    header.author        = 1;
                    header.receipt       = 1;
                    header.msg_type      = msg->msg_type;

                    size_t length = sizeof(header);
                    uint8_t *data = calloc(1, length);
                    memcpy(data, &header, sizeof(header));

                    char *hex = get_friend(m->id)->id_str;
                    if (msg->disk_offset) {
                        LOG_TRACE("Messages", "Updating message -> disk_offset is %lu" , msg->disk_offset);
                        utox_update_chatlog(hex, msg->disk_offset, data, length);
                    } else if (msg->disk_offset == 0 && start <= 1 && receipt_number == 1) {
                        /* This could get messy if receipt is 1 msg position is 0 and the offset is actually wrong,
                         * But I couldn't come up with any other way to verify the rare case of a bad offset
                         * start <= 1 to offset for the day change notification                                    */
                        LOG_TRACE("Messages", "Updating first message -> disk_offset is %lu" , msg->disk_offset);
                        utox_update_chatlog(hex, msg->disk_offset, data, length);
                    } else {
                        LOG_ERR("Messages", "Messages:\tUnable to update this message...\n"
                                    "\t\tmsg->disk_offset %lu && m->number %u receipt_number %u \n",
                                    msg->disk_offset, m->number, receipt_number);
                    }
                    free(data);

                    postmessage_utox(FRIEND_MESSAGE_UPDATE, 0, 0, NULL); /* Used to redraw the screen */
                    pthread_mutex_unlock(&messages_lock);
                    return;
                }
            }
        }
    }
    LOG_ERR("Messages", "Received a receipt for a message we don't have a record of. %u" , receipt_number);
    pthread_mutex_unlock(&messages_lock);
}

static void messages_draw_timestamp(int x, int y, const time_t *time) {
    struct tm *ltime = localtime(time);

    char     timestr[9];
    uint16_t len;


    if (settings.use_long_time_msg) {
        len = snprintf(timestr, sizeof(timestr), "%.2u:%.2u:%.2u", ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
        x -= textwidth("24:60:00", sizeof "24:60:00" - 1);
    } else {
        len = snprintf(timestr, sizeof(timestr), "%u:%.2u", ltime->tm_hour, ltime->tm_min);
        x -= textwidth("24:60", sizeof "24:60" - 1);
    }

    if (len >= sizeof(timestr)) {
        len = sizeof(timestr) - 1;
    }

    setcolor(COLOR_MAIN_TEXT_SUBTEXT);
    setfont(FONT_MISC);
    drawtext(x, y, timestr, len);
}

static void messages_draw_author(int x, int y, int w, char *name, uint32_t length, uint32_t color) {
    setcolor(color);
    setfont(FONT_TEXT);
    drawtextwidth_right(x, w, y, name, length);
}

static int messages_draw_text(const char *msg, size_t length, uint32_t msg_height, uint8_t msg_type, bool author,
                              bool receipt, uint16_t highlight_start, uint16_t highlight_end,
                              int x, int y, int w, int UNUSED(h))
{
    switch (msg_type) {
        case MSG_TYPE_TEXT: {
            if (author) {
                if (receipt) {
                    setcolor(COLOR_MSG_USER);
                } else {
                    setcolor(COLOR_MSG_USER_PEND);
                }
            } else {
                setcolor(COLOR_MSG_CONTACT);
            }
            break;
        }
        case MSG_TYPE_NOTICE:
        case MSG_TYPE_NOTICE_DAY_CHANGE:
        case MSG_TYPE_ACTION_TEXT: {
            setcolor(COLOR_MAIN_TEXT_ACTION);
            break;
        }
    }

    setfont(FONT_TEXT);

    int ny = utox_draw_text_multiline_within_box(x, y, w + x, MAIN_TOP, y + msg_height, font_small_lineheight, msg,
                                                 length, highlight_start, highlight_end, 0, 0, 1);

    if (ny < y || (uint32_t)(ny - y) + MESSAGES_SPACING != msg_height) {
        LOG_TRACE("Messages", "Text Draw Error:\ty %i | ny %i | mheight %u | width %i " , y, ny, msg_height, w);
    }

    return ny;
}

/* draws an inline image at rect (x,y,width,height)
 *  maxwidth is maximum width the image can take in
 *  zoom is whether the image is currently zoomed in
 *  position is the y position along the image the player has scrolled */
static int messages_draw_image(MSG_IMG *img, int x, int y, uint32_t maxwidth) {
    image_set_filter(img->image, FILTER_BILINEAR);

    if (!img->zoom && img->w > maxwidth) {
        image_set_scale(img->image, (double)maxwidth / img->w);

        draw_image(img->image, x, y, maxwidth, img->h * maxwidth / img->w, 0, 0);

        image_set_scale(img->image, 1.0);
    } else if (img->w > maxwidth) {
        draw_image(img->image, x, y, maxwidth, img->h, (int)((double)(img->w - maxwidth) * img->position), 0);
    } else {
        draw_image(img->image, x, y, img->w, img->h, 0, 0);
    }

    return (img->zoom || img->w <= maxwidth) ? img->h : img->h * maxwidth / img->w;
}

/* Draw macros added, to reduce future line edits. */
#define DRAW_FT_RECT(color) draw_rect_fill(dx, y, d_width, FILE_TRANSFER_BOX_HEIGHT, color)

#define DRAW_FT_PROG(color) draw_rect_fill(dx, y, prog_bar, FILE_TRANSFER_BOX_HEIGHT, color)

#define DRAW_FT_CAP(bg, fg)                                                                                 \
    do {                                                                                                    \
        drawalpha(BM_FT_CAP, dx - room_for_clip, y, BM_FT_CAP_WIDTH, BM_FTB_HEIGHT, bg);                    \
        drawalpha(BM_FILE, dx - room_for_clip + SCALE(4), y + SCALE(4), BM_FILE_WIDTH, BM_FILE_HEIGHT, fg); \
    } while (0)

/* Always first */
#define DRAW_FT_NO_BTN()                                                                        \
    do {                                                                                        \
        drawalpha(BM_FTB1, btnx, tbtn_bg_y, btn_bg_w, tbtn_bg_h,                                \
                  (mouse_left_btn ? COLOR_BTN_DANGER_BKGRND_HOVER : COLOR_BTN_SUCCESS_BKGRND)); \
        drawalpha(BM_NO, btnx + ((btn_bg_w - btnw) / 2), tbtn_y, btnw, btnh,                    \
                  (mouse_left_btn ? COLOR_BTN_DANGER_TEXT_HOVER : COLOR_BTN_DANGER_TEXT));      \
    } while (0)

/* Always last */
#define DRAW_FT_YES_BTN()                                                                           \
    do {                                                                                            \
        drawalpha(BM_FTB2, btnx + btn_bg_w + SCALE(2), tbtn_bg_y, btn_bg_w, tbtn_bg_h,              \
                  (mouse_rght_btn ? COLOR_BTN_SUCCESS_BKGRND_HOVER : COLOR_BTN_SUCCESS_BKGRND));    \
        drawalpha(BM_YES, btnx + btn_bg_w + SCALE(2) + ((btn_bg_w - btnw) / 2), tbtn_y, btnw, btnh, \
                  (mouse_rght_btn ? COLOR_BTN_SUCCESS_TEXT_HOVER : COLOR_BTN_SUCCESS_TEXT));        \
    } while (0)

#define DRAW_FT_PAUSE_BTN()                                                                           \
    do {                                                                                              \
        drawalpha(BM_FTB2, btnx + btn_bg_w + SCALE(2), tbtn_bg_y, btn_bg_w, tbtn_bg_h,                \
                  (mouse_rght_btn ? COLOR_BTN_SUCCESS_BKGRND_HOVER : COLOR_BTN_SUCCESS_BKGRND));      \
        drawalpha(BM_PAUSE, btnx + btn_bg_w + SCALE(2) + ((btn_bg_w - btnw) / 2), tbtn_y, btnw, btnh, \
                  (mouse_rght_btn ? COLOR_BTN_SUCCESS_TEXT_HOVER : COLOR_BTN_SUCCESS_TEXT));          \
    } while (0)

#define DRAW_FT_RESUME_BTN()                                                                           \
    do {                                                                                               \
        drawalpha(BM_FTB2, btnx + btn_bg_w + SCALE(2), tbtn_bg_y, btn_bg_w, tbtn_bg_h,                 \
                  (mouse_rght_btn ? COLOR_BTN_SUCCESS_BKGRND_HOVER : COLOR_BTN_SUCCESS_BKGRND));       \
        drawalpha(BM_RESUME, btnx + btn_bg_w + SCALE(2) + ((btn_bg_w - btnw) / 2), tbtn_y, btnw, btnh, \
                  (mouse_rght_btn ? COLOR_BTN_SUCCESS_TEXT_HOVER : COLOR_BTN_SUCCESS_TEXT));           \
    } while (0)

#define DRAW_FT_TEXT_RIGHT(str, len)                   \
    do {                                               \
        wbound -= (textwidth(str, len) + (SCALE(12))); \
        drawtext(wbound, y + SCALE(8), str, len);      \
    } while (0)

#define DRAW_FT_ALPH_RIGHT(bm, col)                     \
    do {                                                \
        wbound -= btnw + (SCALE(12));                   \
        drawalpha(bm, wbound, tbtn_y, btnw, btnh, col); \
    } while (0)

#define DRAWSTR_FT_RIGHT(t) DRAW_FT_TEXT_RIGHT(S(t), SLEN(t))


static void messages_draw_filetransfer(MESSAGES *m, MSG_FILE *file, uint32_t i, int x, int y, int w, int UNUSED(h)) {
    // Used in macros.
    int room_for_clip = BM_FT_CAP_WIDTH + SCALE(2);
    int dx            = x + MESSAGES_X + room_for_clip;
    int d_width       = w - MESSAGES_X - get_time_width() - room_for_clip;
    /* Mouse Positions */
    bool mo             = (m->cursor_over_msg == i);
    bool mouse_over     = (mo && m->cursor_over_position) ? 1 : 0;
    bool mouse_rght_btn = (mo && m->cursor_over_position == 2) ? 1 : 0;
    bool mouse_left_btn = (mo && m->cursor_over_position == 1) ? 1 : 0;

    /* Button Background */
    int btn_bg_w = BM_FTB_WIDTH;
    /* Button Background heights */
    int tbtn_bg_y = y;
    int tbtn_bg_h = BM_FTB_HEIGHT;
    /* Top button info */
    int btnx   = dx + d_width - (btn_bg_w * 2) - SCALE(2);
    int tbtn_y = y + SCALE(8);
    int btnw   = BM_FB_WIDTH;
    int btnh   = BM_FB_HEIGHT;

    long double file_percent = (double)file->progress / (double)file->size;
    if (file->progress > file->size) {
        file->progress = file->size;
        file_percent = 1.0;
    }

    int max = file->name_length + 128;
    char ft_text[max];
    char *text = ft_text;

    text += snprintf(text, max, "%.*s ", (int)file->name_length, file->name);
    text += sprint_humanread_bytes(text, text - ft_text, file->size);

    setfont(FONT_MISC);
    setcolor(COLOR_BKGRND_MAIN);

    int wbound = dx + d_width - SCALE(6);

    switch (file->file_status) {
        case FILE_TRANSFER_STATUS_NONE:
        case FILE_TRANSFER_STATUS_ACTIVE:
        case FILE_TRANSFER_STATUS_PAUSED_US:
        case FILE_TRANSFER_STATUS_PAUSED_BOTH:
        case FILE_TRANSFER_STATUS_PAUSED_THEM: {
            int ftb_allowance = (BM_FTB_WIDTH * 2) + (SCALE(4));
            d_width -= ftb_allowance;
            wbound -= ftb_allowance;
            break;
        }

        default: {
            // we'll round the corner even without buttons.
            d_width -= btn_bg_w;
            break;
        }
    }

    // progress rectangle
    uint32_t prog_bar = (file->size == 0) ? 0 : ((long double)d_width * file_percent);

    switch (file->file_status) {
        case FILE_TRANSFER_STATUS_COMPLETED: {
            /* If mouse over use hover color */
            uint32_t text       = mouse_over ? COLOR_BTN_SUCCESS_TEXT_HOVER : COLOR_BTN_SUCCESS_TEXT,
                     background = mouse_over ? COLOR_BTN_SUCCESS_BKGRND_HOVER : COLOR_BTN_SUCCESS_BKGRND;

            setcolor(text);
            DRAW_FT_CAP(background, text);
            DRAW_FT_RECT(background);
            drawalpha(BM_FTB2, dx + d_width, tbtn_bg_y, btn_bg_w, tbtn_bg_h, background);

            if (file->inline_png) {
                DRAWSTR_FT_RIGHT(CLICKTOSAVE);
            } else {
                DRAWSTR_FT_RIGHT(CLICKTOOPEN);
            }
            DRAW_FT_ALPH_RIGHT(BM_YES, text);
            break;
        }

        case FILE_TRANSFER_STATUS_KILLED: {
            setcolor(COLOR_BTN_DANGER_TEXT);
            DRAW_FT_CAP(COLOR_BTN_DANGER_BACKGROUND, COLOR_BTN_DANGER_TEXT);
            DRAW_FT_RECT(COLOR_BTN_DANGER_BACKGROUND);
            drawalpha(BM_FTB2, dx + d_width, tbtn_bg_y, btn_bg_w, tbtn_bg_h, COLOR_BTN_DANGER_BACKGROUND);

            DRAWSTR_FT_RIGHT(TRANSFER_CANCELLED);
            DRAW_FT_ALPH_RIGHT(BM_NO, COLOR_BTN_DANGER_TEXT);
            break;
        }

        case FILE_TRANSFER_STATUS_BROKEN: {
            setcolor(COLOR_BTN_DANGER_TEXT);
            DRAW_FT_CAP(COLOR_BTN_DANGER_BACKGROUND, COLOR_BTN_DANGER_TEXT);
            DRAW_FT_RECT(COLOR_BTN_DANGER_BACKGROUND);
            drawalpha(BM_FTB2, dx + d_width, tbtn_bg_y, btn_bg_w, tbtn_bg_h, COLOR_BTN_DANGER_BACKGROUND);

            DRAWSTR_FT_RIGHT(TRANSFER_BROKEN);
            DRAW_FT_ALPH_RIGHT(BM_NO, COLOR_BTN_DANGER_TEXT);
            break;
        }

        case FILE_TRANSFER_STATUS_NONE: {
            /* ↑ used for incoming transfers */
            setcolor(COLOR_BTN_DISABLED_TRANSFER);
            DRAW_FT_CAP(COLOR_BTN_DISABLED_BKGRND, COLOR_BTN_DISABLED_TRANSFER);
            DRAW_FT_RECT(COLOR_BTN_DISABLED_BKGRND);

            DRAW_FT_NO_BTN();
            DRAW_FT_YES_BTN();

            DRAW_FT_PROG(COLOR_BTN_DISABLED_FORGRND);
            break;
        }

        case FILE_TRANSFER_STATUS_ACTIVE: {
            setcolor(COLOR_BTN_INPROGRESS_TEXT);
            DRAW_FT_CAP(COLOR_BTN_INPROGRESS_BKGRND, COLOR_BTN_INPROGRESS_TEXT);
            DRAW_FT_RECT(COLOR_BTN_INPROGRESS_BKGRND);

            DRAW_FT_NO_BTN();
            DRAW_FT_PAUSE_BTN();

            char speed[32] = {0};
            char *p = speed + sprint_humanread_bytes(speed, 32, file->speed);
            p += snprintf(p, speed - p, "/s %lus",
                               file->speed ? (file->size - file->progress) / file->speed : 0);
            DRAW_FT_TEXT_RIGHT(speed, p - speed);

            DRAW_FT_PROG(COLOR_BTN_INPROGRESS_FORGRND);
            break;
        }

        case FILE_TRANSFER_STATUS_PAUSED_US:
        case FILE_TRANSFER_STATUS_PAUSED_BOTH:
        case FILE_TRANSFER_STATUS_PAUSED_THEM: {
            setcolor(COLOR_BTN_DISABLED_TRANSFER);

            DRAW_FT_CAP(COLOR_BTN_DISABLED_BKGRND, COLOR_BTN_DISABLED_TRANSFER);
            DRAW_FT_RECT(COLOR_BTN_DISABLED_BKGRND);

            DRAW_FT_NO_BTN();

            if (file->file_status == FILE_TRANSFER_STATUS_PAUSED_BOTH
                || file->file_status == FILE_TRANSFER_STATUS_PAUSED_US) {
                /* Paused by at least us */
                DRAW_FT_RESUME_BTN();
            } else {
                /* Paused only by them */
                DRAW_FT_PAUSE_BTN();
            }

            DRAW_FT_PROG(COLOR_BTN_DISABLED_FORGRND);
            break;
        }
    }

    setfont(FONT_TEXT);
    drawtextrange(dx + SCALE(10), wbound - SCALE(10), y + SCALE(6), ft_text, text - ft_text);
}

/* This is a bit hacky, and likely would benifit from being moved to a whole new section including seperating
 * group messages/functions from friend messages and functions from inside ui.c.
 *
 * Idealy group and friend messages wouldn't even need to know about eachother.   */
static int messages_draw_group(MESSAGES *m, MSG_HEADER *msg, uint32_t curr_msg_i, int x, int y, int width, int height) {
    uint32_t h1 = UINT32_MAX, h2 = UINT32_MAX;
    if ((m->sel_start_msg > curr_msg_i && m->sel_end_msg > curr_msg_i)
        || (m->sel_start_msg < curr_msg_i && m->sel_end_msg < curr_msg_i)) {
        /* Out side the highlight area */
        h1 = UINT32_MAX;
        h2 = UINT32_MAX;
    } else {
        if (m->sel_start_msg < curr_msg_i) {
            h1 = 0;
        } else {
            h1 = m->sel_start_position;
        }

        if (m->sel_end_msg > curr_msg_i) {
            h2 = msg->via.grp.length;
        } else {
            h2 = m->sel_end_position;
        }
    }

    /* error check */
    if ((m->sel_start_msg == m->sel_end_msg && m->sel_start_position == m->sel_end_position) || h1 == h2) {
        h1 = UINT32_MAX;
        h2 = UINT32_MAX;
    }

    messages_draw_author(x, y, MESSAGES_X - NAME_OFFSET, msg->via.grp.author, msg->via.grp.author_length, msg->via.grp.author_color);
    messages_draw_timestamp(x + width, y, &msg->time);
    return messages_draw_text(msg->via.grp.msg, msg->via.grp.length, msg->height, msg->msg_type, msg->our_msg, 1,
                              h1, h2, x + MESSAGES_X, y, width - get_time_width() - MESSAGES_X, height)
           + MESSAGES_SPACING;
}

static int messages_time_change(MESSAGES *m, MSG_HEADER *msg, size_t index, int x, int y, int width, int height) {
    uint32_t h1 = UINT32_MAX, h2 = UINT32_MAX;

    if ((m->sel_start_msg > index && m->sel_end_msg > index)
        || (m->sel_start_msg < index && m->sel_end_msg < index)) {
        /* Out side the highlight area */
        h1 = UINT32_MAX;
        h2 = UINT32_MAX;
    } else {
        if (m->sel_start_msg < index) {
            h1 = 0;
        } else {
            h1 = m->sel_start_position;
        }

        if (m->sel_end_msg > index) {
            h2 = msg->via.notice.length;
        } else {
            h2 = m->sel_end_position;
        }
    }

                /* error check */
    if ((m->sel_start_msg == m->sel_end_msg && m->sel_start_position == m->sel_end_position) || h1 == h2) {
        h1 = UINT32_MAX;
        h2 = UINT32_MAX;
    }

    /* text.c is super broken, so we have to be hacky here */
    if (h2 != msg->via.notice.length) {
        if (m->sel_end_msg != index) {
            h2 = msg->via.notice.length - h2;
        } else {
            h2 -= h1;
        }
    }

    return messages_draw_text(msg->via.notice.msg, msg->via.notice.length, msg->height,
                              msg->msg_type, msg->our_msg, msg->receipt_time,
                              h1, h2, x + MESSAGES_X, y, width - get_time_width() - MESSAGES_X, height);
}

/** Formats all messages from self and friends, and then call draw functions
 * to write them to the UI.
 *
 * accepts: messages struct *pointer, int x,y positions, int width,height
 */
void messages_draw(PANEL *panel, int x, int y, int width, int height) {
    if (width - MESSAGES_X - get_time_width() <= 0) {
        return;
    }

    pthread_mutex_lock(&messages_lock);

    MESSAGES *m = panel->object;
    // Do not draw author name next to every message
    uint8_t lastauthor = 0xFF;

    // Message iterator
    MSG_HEADER **p = m->data;
    uint32_t n = m->number;

    if (m->width != width) {
        m->width = width;
        messages_updateheight(m, width - MESSAGES_X + get_time_width());
        y -= scroll_gety(panel->content_scroll, height);
    }

    // Go through messages
    for (size_t curr_msg_i = 0; curr_msg_i != n; curr_msg_i++) {
        MSG_HEADER *msg = *p++;

        /* Decide if we should even bother drawing this message. */
        if (msg->height == 0) {
            /* Empty message */
            pthread_mutex_unlock(&messages_lock);
            return;
        } else if (y + msg->height <= (unsigned)MAIN_TOP) {
            /* message is exclusively above the viewing window */
            y += msg->height;
            continue;
        } else if (y >= height + SCALE(100)) { // NOTE: should not be constant 100
            /* Message is exclusively below the viewing window */
            break;
        }

        // Draw the names for groups or friends
        if (m->is_groupchat) {
            y = messages_draw_group(m, msg, curr_msg_i, x, y, width, height);
            continue;
        } else {
            bool draw_author = true;
            switch (msg->msg_type) {
                case MSG_TYPE_NULL: {
                    // This shouldn't happen.
                    LOG_ERR("Messages", "Invalid message type in messages_draw.");
                    break;
                }

                case MSG_TYPE_ACTION_TEXT: {
                    // Always draw name next to action message
                    lastauthor = ~0;
                    break;
                }

                case MSG_TYPE_TEXT:
                case MSG_TYPE_IMAGE:
                case MSG_TYPE_FILE: {
                    draw_author = true;
                    break;
                }

                case MSG_TYPE_NOTICE:
                case MSG_TYPE_NOTICE_DAY_CHANGE: {
                    draw_author = false;
                    break;
                }
            }

            if (draw_author) {
                if (msg->our_msg != lastauthor) {
                    FRIEND *f = get_friend(m->id);
                    if (msg->our_msg) {
                        messages_draw_author(x, y, MESSAGES_X - NAME_OFFSET, self.name, self.name_length,
                                             COLOR_MAIN_TEXT_SUBTEXT);
                    } else if (f->alias) {
                        messages_draw_author(x, y, MESSAGES_X - NAME_OFFSET, f->alias, f->alias_length,
                                             COLOR_MAIN_TEXT_CHAT);
                    } else {
                        messages_draw_author(x, y, MESSAGES_X - NAME_OFFSET, f->name, f->name_length,
                                             COLOR_MAIN_TEXT_CHAT);
                    }
                    lastauthor = msg->our_msg;
                }
            }
        }

        // Draw message contents
        switch (msg->msg_type) {
            case MSG_TYPE_NULL: {
                LOG_ERR("Messages", "Error msg type is null");
                break;
            }

            case MSG_TYPE_TEXT:
            case MSG_TYPE_ACTION_TEXT:
            case MSG_TYPE_NOTICE: {
                // Draw timestamps
                messages_draw_timestamp(x + width, y, &msg->time);
                y = messages_time_change(m, msg, curr_msg_i, x, y, width, height);
                break;
            }
            case MSG_TYPE_NOTICE_DAY_CHANGE: {
                y = messages_time_change(m, msg, curr_msg_i, x, y, width, height);
                break;
            }

            // Draw image
            case MSG_TYPE_IMAGE: {
                y += messages_draw_image(&msg->via.img, x + MESSAGES_X, y, width - MESSAGES_X - get_time_width());
                break;
            }

            // Draw file transfer
            case MSG_TYPE_FILE: {
                messages_draw_filetransfer(m, &msg->via.ft, curr_msg_i, x, y, width, height);
                y += FILE_TRANSFER_BOX_HEIGHT;
                break;
            }
        }

        y += MESSAGES_SPACING;
    }

    pthread_mutex_unlock(&messages_lock);
}

static bool messages_mmove_text(MESSAGES *m, int width, int mx, int my, int dy, char *message, uint32_t msg_height,
                                uint16_t msg_length)
{
    cursor                  = CURSOR_TEXT;
    m->cursor_over_position = hittextmultiline(mx - MESSAGES_X, width - MESSAGES_X - get_time_width(), (my < 0 ? 0 : my),
                                               msg_height, font_small_lineheight, message, msg_length, 1);

    if (my < 0 || my >= dy || mx < MESSAGES_X || m->cursor_over_position == msg_length) {
        m->cursor_over_uri = UINT32_MAX;
        return 0;
    }

    bool prev_cursor_down_uri = m->cursor_down_uri;

    if (m->cursor_over_uri != UINT32_MAX) {
        m->cursor_down_uri = 0;
        m->cursor_over_uri = UINT32_MAX;
    }

    /* Seek back to the last word/line break */
    char *str = message + m->cursor_over_position;
    while (str != message) {
        str--;
        if (*str == ' ' || *str == '\n') {
            str++;
            break;
        }
    }

    /* Check if it's a URI we handle TODO: handle moar! */
    char *end = message + msg_length;
    while (str != end && *str != ' ' && *str != '\n') {
        if (str == message || *(str - 1) == '\n' || *(str - 1) == ' ') {
            if (m->cursor_over_uri == UINT32_MAX && end - str >= 7 && (strncmp(str, "http://", 7) == 0)) {
                cursor             = CURSOR_HAND;
                m->cursor_over_uri = str - message;
            } else if (m->cursor_over_uri == UINT32_MAX && end - str >= 8 && (strncmp(str, "https://", 8) == 0)) {
                cursor             = CURSOR_HAND;
                m->cursor_over_uri = str - message;
            } else if (m->cursor_over_uri == UINT32_MAX && end - str >= 4 && (strncmp(str, "tox:", 4) == 0)) {
                cursor             = CURSOR_HAND;
                m->cursor_over_uri = str - message;
            }
        }
        str++;
    }

    if (m->cursor_over_uri != UINT32_MAX) {
        m->urllen          = (str - message) - m->cursor_over_uri;
        m->cursor_down_uri = prev_cursor_down_uri;
        LOG_TRACE("Messages", "urllen %u" , m->urllen);
    }

    return 0;
}

static bool messages_mmove_image(MSG_IMG *image, uint32_t max_width, int mx, int my) {
    if (image->w > max_width) {
        mx -= MESSAGES_X;
        int w = image->w > max_width ? max_width : image->w;
        int h = (image->zoom || image->w <= max_width) ? image->h : image->h * max_width / image->w;

        if (mx >= 0 && my >= 0 && mx < w && my < h) {
            cursor = CURSOR_ZOOM_IN + image->zoom;
            return 1;
        }
    }
    return 0;
}

static uint8_t messages_mmove_filetransfer(int mx, int my, int width) {
    mx -= SCALE(10); /* Why? */
    if (mx >= 0 && mx < width && my >= 0 && my < FILE_TRANSFER_BOX_HEIGHT) {
        if (mx >= width - get_time_width() - (BM_FTB_WIDTH * 2) - SCALE(2) - SCROLL_WIDTH
            && mx <= width - get_time_width() - SCROLL_WIDTH) {
            if (mx >= width - get_time_width() - BM_FTB_WIDTH - SCROLL_WIDTH) {
                // mouse is over the right button (pause / accept)
                return 2;
            } else {
                // mouse is over the left button  (cancel)
                return 1;
            }
        }
        return 3;
    }
    return 0;
}

bool messages_mmove(PANEL *panel, int UNUSED(px), int UNUSED(py), int width, int UNUSED(height), int mx, int my, int dx,
                    int UNUSED(dy))
{
    MESSAGES *m = panel->object;

    if (mx >= width - get_time_width()) {
        m->cursor_over_time = 1;
    } else {
        m->cursor_over_time = 0;
    }

    if (m->cursor_down_msg < m->number) {
        uint32_t maxwidth = width - MESSAGES_X - get_time_width();
        MSG_HEADER *msg = m->data[m->cursor_down_msg];
        if ((msg->msg_type == MSG_TYPE_IMAGE) && (msg->via.img.w > maxwidth)) {
            msg->via.img.position -= (double)dx / (double)(msg->via.img.w - maxwidth);
            if (msg->via.img.position > 1.0) {
                msg->via.img.position = 1.0;
            } else if (msg->via.img.position < 0.0) {
                msg->via.img.position = 0.0;
            }
            cursor = CURSOR_ZOOM_OUT;
            return true;
        }
    }

    if (mx < 0 || my < 0 || my > m->height) {
        if (m->cursor_over_msg != UINT32_MAX) {
            m->cursor_over_msg = UINT32_MAX;
            return true;
        }
        return false;
    }

    setfont(FONT_TEXT);

    MSG_HEADER **p = m->data;
    uint32_t i = 0;
    bool     need_redraw = false;

    while (i < m->number) {
        MSG_HEADER *msg = *p++;

        int dy = msg->height; /* dy is the wrong name here, you should change it! */

        if (my >= 0 && my < dy) {
            m->cursor_over_msg = i;

            switch (msg->msg_type) {
                case MSG_TYPE_NULL: {
                    LOG_ERR("Messages", "Invalid message type in messages_mmove.");
                    return false;
                }

                case MSG_TYPE_TEXT:
                case MSG_TYPE_ACTION_TEXT:
                case MSG_TYPE_NOTICE:
                case MSG_TYPE_NOTICE_DAY_CHANGE: {
                    if (m->is_groupchat) {
                        messages_mmove_text(m, width, mx, my, dy, msg->via.grp.msg,
                                            msg->height, msg->via.grp.length);
                    } else {
                        messages_mmove_text(m, width, mx, my, dy, msg->via.txt.msg,
                                            msg->height, msg->via.txt.length);
                    }
                    if (m->cursor_down_msg != UINT32_MAX
                        && (m->cursor_down_position != m->cursor_over_position
                            || m->cursor_down_msg != m->cursor_over_msg))
                    {
                        m->selecting_text = 1;
                    }
                    break;
                }

                case MSG_TYPE_IMAGE: {
                    m->cursor_over_position = messages_mmove_image(&msg->via.img, (width - MESSAGES_X - get_time_width()), mx, my);
                    break;
                }

                case MSG_TYPE_FILE: {
                    m->cursor_over_position = messages_mmove_filetransfer(mx, my, width);
                    if (m->cursor_over_position) {
                        need_redraw = true;
                    }
                    break;
                }
            }

            if (i != m->cursor_over_msg && m->cursor_over_msg != UINT32_MAX
                && (msg->msg_type == MSG_TYPE_FILE || m->data[m->cursor_over_msg]->msg_type == MSG_TYPE_FILE)) {
                need_redraw = true; // Redraw file on hover-in/out.
            }

            if (m->selecting_text) {
                need_redraw = true;

                if (m->cursor_down_msg != m->cursor_over_msg || m->cursor_down_position <= m->cursor_over_position) {
                    m->sel_start_position = m->cursor_down_position;
                    m->sel_end_position   = m->cursor_over_position;
                } else {
                    m->sel_start_position = m->cursor_over_position;
                    m->sel_end_position   = m->cursor_down_position;
                }

                if (m->cursor_down_msg <= m->cursor_over_msg) {
                    m->sel_start_msg = m->cursor_down_msg;
                    m->sel_end_msg   = m->cursor_over_msg;
                } else {
                    m->sel_start_msg      = m->cursor_over_msg;
                    m->sel_end_msg        = m->cursor_down_msg;
                    m->sel_start_position = m->cursor_over_position;
                    m->sel_end_position   = m->cursor_down_position;
                }
            }

            return need_redraw;
        }

        my -= dy;

        i++;
    }

    return false;
}

bool messages_mdown(PANEL *panel) {
    MESSAGES *m        = panel->object;
    m->cursor_down_msg = UINT32_MAX;

    if (m->cursor_over_msg != UINT32_MAX) {
        MSG_HEADER *msg = m->data[m->cursor_over_msg];
        switch (msg->msg_type) {
            case MSG_TYPE_NULL: {
                LOG_ERR("Messages", "Invalid message type in messages_mdown.");
                return false;
            }

            case MSG_TYPE_TEXT:
            case MSG_TYPE_ACTION_TEXT:
            case MSG_TYPE_NOTICE:
            case MSG_TYPE_NOTICE_DAY_CHANGE: {
                if (m->cursor_over_uri != UINT32_MAX) {
                    m->cursor_down_uri = m->cursor_over_uri;
                    LOG_TRACE("Messages", "mdn dURI %u, oURI %u" , m->cursor_down_uri, m->cursor_over_uri);
                }

                m->sel_start_msg = m->sel_end_msg = m->cursor_down_msg = m->cursor_over_msg;
                m->sel_start_position = m->sel_end_position = m->cursor_down_position = m->cursor_over_position;
                break;
            }

            case MSG_TYPE_IMAGE: {
                if (m->cursor_over_position) {
                    if (!msg->via.img.zoom) {
                        msg->via.img.zoom = 1;
                        message_updateheight(m, msg);
                    } else {
                        m->cursor_down_msg = m->cursor_over_msg;
                    }
                }
                break;
            }

            case MSG_TYPE_FILE: {
                if (m->cursor_over_position == 0) {
                    break;
                }

                FRIEND *f = get_friend(m->id);
                FILE_TRANSFER *ft;
                uint32_t ft_number = msg->via.ft.file_number;
                if (ft_number >= (1 << 16)) {
                    ft = &f->ft_incoming[(ft_number >> 16) - 1]; // TODO, abstraction needed
                } else {
                    ft = &f->ft_outgoing[ft_number]; // TODO, abstraction needed
                }

                if (msg->via.ft.file_status == FILE_TRANSFER_STATUS_COMPLETED) {
                    if (m->cursor_over_position) {
                        if (msg->via.ft.inline_png) {
                            file_save_inline_image_png(msg);
                        } else {
                            openurl((char *)msg->via.ft.path);
                        }
                    }

                    return true;
                }

                if (m->cursor_over_position == 2) { // Right button, should be accept/pause/resume
                    if (!msg->our_msg && msg->via.ft.file_status == FILE_TRANSFER_STATUS_NONE) {
                        native_select_dir_ft(m->id, msg->via.ft.file_number, ft);
                        return true;
                    }

                    if (msg->via.ft.file_status == FILE_TRANSFER_STATUS_ACTIVE) {
                        postmessage_toxcore(TOX_FILE_PAUSE, m->id, msg->via.ft.file_number, ft);
                    } else {
                        postmessage_toxcore(TOX_FILE_RESUME, m->id, msg->via.ft.file_number, ft);
                    }
                } else if (m->cursor_over_position == 1) { // Should be cancel
                    postmessage_toxcore(TOX_FILE_CANCEL, m->id, msg->via.ft.file_number, ft);
                }

                return true;
            }
        }

        return true;
    } else if (m->sel_start_msg != m->sel_end_msg || m->sel_start_position != m->sel_end_position) {
        m->sel_start_msg      = 0;
        m->sel_end_msg        = 0;
        m->sel_start_position = 0;
        m->sel_end_position   = 0;

        return true;
    }

    return false;
}

bool messages_dclick(PANEL *panel, bool triclick) {
    MESSAGES *m = panel->object;

    if (m->cursor_over_time) {
        settings.use_long_time_msg = !settings.use_long_time_msg;
        return true;
    }

    if (m->cursor_over_msg != UINT32_MAX) {
        MSG_HEADER *msg = m->data[m->cursor_over_msg];

        switch (msg->msg_type) {
            case MSG_TYPE_NULL: {
                LOG_ERR("Messages", "Invalid message type in messages_dclick.");
                return false;
            }

            case MSG_TYPE_FILE:
            case MSG_TYPE_NOTICE:
            case MSG_TYPE_NOTICE_DAY_CHANGE: {
                return false;
            }

            case MSG_TYPE_TEXT:
            case MSG_TYPE_ACTION_TEXT: {
                m->sel_start_msg = m->sel_end_msg = m->cursor_over_msg;

                const char c = triclick ? '\n' : ' ';

                uint16_t i = m->cursor_over_position;
                while (i != 0 && msg->via.txt.msg[i - 1] != c) {
                    i -= utf8_unlen(msg->via.txt.msg + i);
                }
                m->sel_start_position = i;
                i = m->cursor_over_position;
                while (i != msg->via.txt.length && msg->via.txt.msg[i] != c) {
                    i += utf8_len(msg->via.txt.msg + i);
                }
                m->sel_end_position = i;

                uint32_t diff = m->sel_end_position - m->sel_start_position;
                setselection(msg->via.txt.msg + m->sel_start_position, diff);

                return true;
            }

            case MSG_TYPE_IMAGE: {
                if (m->cursor_over_position) {
                    if (msg->via.img.zoom) {
                        msg->via.img.zoom = 0;
                        message_updateheight(m, msg);
                    }
                }

                return true;
            }
        }
    }

    return false;
}

static void contextmenu_messages_onselect(uint8_t i) {
    copy(!!i); /* if not 0 force a 1 */
}

bool messages_mright(PANEL *panel) {
    const MESSAGES *m = panel->object;
    if (m->cursor_over_msg == UINT32_MAX) {
        return false;
    }

    const MSG_HEADER *msg = m->data[m->cursor_over_msg];

    switch (msg->msg_type) {
        case MSG_TYPE_NULL: {
            LOG_ERR("Messages", "Invalid message type in messages_mdown.");
            return false;
        }

        case MSG_TYPE_TEXT:
        case MSG_TYPE_ACTION_TEXT: {
            static UTOX_I18N_STR menu_copy[] = { STR_COPY, STR_COPY_WITH_NAMES };
            contextmenu_new(COUNTOF(menu_copy), menu_copy, contextmenu_messages_onselect);
            return true;
        }

        case MSG_TYPE_NOTICE:
        case MSG_TYPE_NOTICE_DAY_CHANGE:
        case MSG_TYPE_IMAGE:
        case MSG_TYPE_FILE: {
            return false;
        }
    }

    LOG_FATAL_ERR(EXIT_FAILURE, "Messages", "Congratulations, you've reached dead code. Please report this.");
}

bool messages_mwheel(PANEL *UNUSED(panel), int UNUSED(height), double UNUSED(d), bool UNUSED(smooth)) {
    return false;
}

bool messages_mup(PANEL *panel) {
    MESSAGES *m = panel->object;

    if (!m->data) {
        return false;
    }

    if (m->cursor_over_msg != UINT32_MAX) {
        MSG_HEADER *msg = m->data[m->cursor_over_msg];
        if (msg->msg_type == MSG_TYPE_TEXT) {
            if (m->cursor_over_uri != UINT32_MAX && m->cursor_down_uri == m->cursor_over_uri
                && m->cursor_over_position >= m->cursor_over_uri
                && m->cursor_over_position <= m->cursor_over_uri + m->urllen - 1 /* - 1 Don't open on white space */
                && !m->selecting_text) {
                LOG_TRACE("Messages", "mup dURI %u, oURI %u" , m->cursor_down_uri, m->cursor_over_uri);
                char url[m->urllen + 1];
                memcpy(url, msg->via.txt.msg + m->cursor_over_uri, m->urllen * sizeof(char));
                url[m->urllen] = 0;
                openurl(url);
                m->cursor_down_uri = 0;
            }
        }
    }

    if (m->selecting_text) {
        const uint32_t max_selection_size = UINT16_MAX + 1;
        char *sel = calloc(1, max_selection_size);
        setselection(sel, messages_selection(panel, sel, max_selection_size, 0));
        free(sel);

        m->selecting_text = 0;
    }

    m->cursor_down_msg = UINT32_MAX;

    return false;
}

bool messages_mleave(PANEL *UNUSED(m)) {
    return false;
}

int messages_selection(PANEL *panel, char *buffer, uint32_t len, bool names) {
    MESSAGES *m = panel->object;

    if (m->number == 0) {
        return 0;
    }

    uint32_t i = m->sel_start_msg, n = m->sel_end_msg + 1;
    MSG_HEADER **dp = &m->data[i];

    char *p = buffer;

    while (i != UINT32_MAX && i != n) {
        const MSG_HEADER *msg = *dp++;

        if (names && (i != m->sel_start_msg || m->sel_start_position == 0)) {
            if (m->is_groupchat) {
                memcpy(p, msg->via.grp.author, msg->via.grp.author_length);
                p += msg->via.grp.author_length;
                len -= msg->via.grp.author_length;
            } else {
                const FRIEND *f = get_friend(m->id);

                if (!msg->our_msg) {
                    if (len <= f->name_length) {
                        break;
                    }

                    memcpy(p, f->name, f->name_length);
                    p += f->name_length;
                    len -= f->name_length;
                } else {
                    if (len <= self.name_length) {
                        break;
                    }

                    memcpy(p, self.name, self.name_length);
                    p += self.name_length;
                    len -= self.name_length;
                }
            }

            if (len <= 2) {
                break;
            }

            strcpy2(p, ": ");
            p += 2;
            len -= 2;
        }

        switch (msg->msg_type) {
            case MSG_TYPE_NULL: {
                LOG_ERR("Messages", "Invalid message type in messages_selection.");
                return 0;
            }

            case MSG_TYPE_TEXT:
            case MSG_TYPE_ACTION_TEXT: {
                char *data;
                uint16_t length;
                if (i == m->sel_start_msg) {
                    if (i == m->sel_end_msg) {
                        data   = msg->via.txt.msg + m->sel_start_position;
                        length = m->sel_end_position - m->sel_start_position;
                    } else {
                        data   = msg->via.txt.msg + m->sel_start_position;
                        length = msg->via.txt.length - m->sel_start_position;
                    }
                } else if (i == m->sel_end_msg) {
                    data   = msg->via.txt.msg;
                    length = m->sel_end_position;
                } else {
                    data   = msg->via.txt.msg;
                    length = msg->via.txt.length;
                }

                if (len <= length) {
                    *p = 0;
                    return p - buffer;
                }

                memcpy(p, data, length);
                p += length;
                len -= length;
                break;
            }

            case MSG_TYPE_IMAGE:
            case MSG_TYPE_FILE:
            case MSG_TYPE_NOTICE:
            case MSG_TYPE_NOTICE_DAY_CHANGE: {
                // Do nothing.
                break;
            }
        }

        i++;

        if (i != n) {
            #ifdef __WIN32__
                if (len <= 2) {
                    break;
                }
                *p++ = '\r';
                *p++ = '\n';
                len -= 2;
            #else
                if (len <= 1) {
                    break;
                }
                *p++ = '\n';
                len--;
            #endif
        }
    }

    return p - buffer;
}

void messages_updateheight(MESSAGES *m, int width) {
    if (!m->data || !width) {
        return;
    }

    setfont(FONT_TEXT);

    uint32_t height = 0;

    for (uint32_t i = 0; i < m->number; ++i) {
        height += message_setheight(m, (void *)m->data[i]);
    }

    m->panel.content_scroll->content_height = m->height = height;
}

bool messages_char(uint32_t ch) {
    MESSAGES *m;

    if (flist_get_friend()) {
        m = messages_friend.object;
    } else if (flist_get_groupchat()) {
        m = messages_group.object;
    } else {
        LOG_TRACE("Messages", "Can't type to nowhere");
        return false;
    }

    switch (ch) {
        // TODO: probabaly need to fix this section :< m->panel.content scroll is likely to be wrong.
        case KEY_PAGEUP: {
            SCROLLABLE *scroll = m->panel.content_scroll;
            scroll->d -= 0.25; // TODO: Change to a full chat-screen height.
            if (scroll->d < 0.0) {
                scroll->d = 0.0;
            }

            return true;
        }

        case KEY_PAGEDOWN: {
            SCROLLABLE *scroll = m->panel.content_scroll;
            scroll->d += 0.25; // TODO: Change to a full chat-screen height.
            if (scroll->d > 1.0) {
                scroll->d = 1.0;
            }

            return true;
        }
    }

    return false;
}

void messages_init(MESSAGES *m, uint32_t friend_number) {
    if (m->data) {
        messages_clear_all(m);
    }

    pthread_mutex_lock(&messages_lock);

    memset(m, 0, sizeof(*m) * COUNTOF(m));

    m->data = calloc(20, sizeof(void *));
    if (!m->data) {
        LOG_FATAL_ERR(EXIT_MALLOC, "Messages", "\n\n\nFATAL ERROR TRYING TO CALLOC FOR MESSAGES.\nTHIS IS A BUG, PLEASE REPORT!\n\n\n");
    }

    m->extra = 20;
    m->id    = friend_number;

    pthread_mutex_unlock(&messages_lock);
}

void message_free(MSG_HEADER *msg) {
    // The group messages are free()d in groups.c (group_free(GROUPCHAT *g))
    switch (msg->msg_type) {
        case MSG_TYPE_NULL: {
            LOG_ERR("Messages", "Invalid message type in message_free.");
            break;
        }

        case MSG_TYPE_IMAGE: {
            image_free(msg->via.img.image);
            break;
        }

        case MSG_TYPE_FILE: {
            free(msg->via.ft.name);
            free(msg->via.ft.path);
            free(msg->via.ft.data);
            break;
        }

        case MSG_TYPE_NOTICE_DAY_CHANGE: {
            free(msg->via.notice_day.msg);
            break;
        }

        case MSG_TYPE_TEXT: {
            free(msg->via.txt.msg);
            break;
        }

        case MSG_TYPE_ACTION_TEXT: {
            free(msg->via.action.msg);
            break;
        }

        case MSG_TYPE_NOTICE: {
            free(msg->via.notice.msg);
            break;
        }
    }

    free(msg);
}

void messages_clear_all(MESSAGES *m) {
    pthread_mutex_lock(&messages_lock);

    for (uint32_t i = 0; i < m->number; i++) {
        message_free(m->data[i]);
    }

    free(m->data);
    m->data   = NULL;
    m->number = 0;
    m->extra  = 0;

    m->sel_start_msg = m->sel_end_msg = m->sel_start_position = m->sel_end_position = 0;

    m->height = 0;
    pthread_mutex_unlock(&messages_lock);
}
