// messages.c

#include "messages.h"

#include "flist.h"
#include "main.h"
#include "theme.h"
#include "util.h"

#include "ui/svg.h"
#include "ui/text.h"

/** Appends a messages from self or friend to the message list;
 * will realloc or trim messages as needed;
 *
 * also handles auto scrolling selections with messages
 *
 * accepts: MESSAGES *pointer, MESSAGE *pointer, MSG_DATA *pointer
 */

static int msgheight(MSG_VOID *msg, int width) {
    switch (msg->msg_type) {
        case MSG_TYPE_TEXT:
        case MSG_TYPE_ACTION_TEXT:
        case MSG_TYPE_NOTICE:
        case MSG_TYPE_NOTICE_DAY_CHANGE: {
            MSG_TEXT *text = (void *)msg;
            int       theight =
                text_height(abs(width - MESSAGES_X - TIME_WIDTH), font_small_lineheight, text->msg, text->length);
            return (theight == 0) ? 0 : theight + MESSAGES_SPACING;
        }

        case MSG_TYPE_IMAGE: {
            MSG_IMG *img      = (void *)msg;
            int      maxwidth = width - MESSAGES_X - TIME_WIDTH;
            return ((img->zoom || img->w <= maxwidth) ? img->h : img->h * maxwidth / img->w) + MESSAGES_SPACING;
        }

        case MSG_TYPE_FILE: {
            return FILE_TRANSFER_BOX_HEIGHT + MESSAGES_SPACING;
        }
    }

    return 0;
}

static int msgheight_group(MSG_GROUP *grp, int width) {
    switch (grp->msg_type) {
        case MSG_TYPE_TEXT:
        case MSG_TYPE_ACTION_TEXT:
        case MSG_TYPE_NOTICE:
        case MSG_TYPE_NOTICE_DAY_CHANGE: {
            int theight = text_height(abs(width - MESSAGES_X - TIME_WIDTH), font_small_lineheight,
                                      grp->msg + grp->author_length, grp->length);
            return (theight == 0) ? 0 : theight + MESSAGES_SPACING;
        }
        default: { debug("Error, can't set this group message height\n"); }
    }

    return 0;
}

static int message_setheight(MESSAGES *m, MSG_VOID *msg) {
    if (m->width == 0) {
        return 0;
    }

    setfont(FONT_TEXT);

    if (m->is_groupchat) {
        MSG_GROUP *grp = (void *)msg;
        msg->height    = msgheight_group(grp, m->width);
    } else {
        msg->height = msgheight(msg, m->width);
    }
    return msg->height;
}

static void message_updateheight(MESSAGES *m, MSG_VOID *msg) {
    if (m->width == 0) {
        return;
    }

    setfont(FONT_TEXT);

    m->height -= msg->height;
    msg->height = message_setheight(m, msg);
    m->height += msg->height;
}

static uint32_t message_add(MESSAGES *m, MSG_VOID *msg) {
    pthread_mutex_lock(&messages_lock);

    /* TODO: test this? */
    if (m->number < UTOX_MAX_BACKLOG_MESSAGES) {
        if (!m->data || m->extra <= 0) {
            if (m->data) {
                m->data = realloc(m->data, (m->number + 10) * sizeof(void *));
                m->extra += 10;
            } else {
                m->data  = calloc(20, sizeof(void *));
                m->extra = 20;
            }

            if (!m->data) {
                debug_error("\n\n\nFATAL ERROR TRYING TO REALLOC FOR MESSAGES.\nTHIS IS A BUG, PLEASE REPORT!\n\n\n");
                exit(30);
            }
        }
        m->data[m->number++] = msg;
        m->extra--;
    } else {
        m->height -= ((MSG_VOID *)m->data[0])->height;
        /* Assuming this is MSG_TEXT is probably a mistake... */
        message_free(m->data[0]);
        memmove(m->data, m->data + 1, (UTOX_MAX_BACKLOG_MESSAGES - 1) * sizeof(void *));
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

    message_updateheight(m, (MSG_VOID *)msg);

    if (m->is_groupchat && flist_get_selected()->data == &group[m->id]) {
        m->panel.content_scroll->content_height = m->height;
    } else if (flist_get_selected()->data == &friend[m->id]) {
        m->panel.content_scroll->content_height = m->height;
    }

    pthread_mutex_unlock(&messages_lock);
    return m->number;
}

static bool msg_add_day_notice(MESSAGES *m, time_t last, time_t next) {

    struct tm *msg_time = 0;
    /* The tm struct is shared, we have to do it this way */
    int ltime_year = 0, ltime_mon = 0, ltime_day = 0;

    msg_time   = localtime(&last);
    ltime_year = msg_time->tm_year;
    ltime_mon  = msg_time->tm_mon;
    ltime_day  = msg_time->tm_mday;
    msg_time   = localtime(&next);

    if (ltime_year < msg_time->tm_year || (ltime_year == msg_time->tm_year && ltime_mon < msg_time->tm_mon)
        || (ltime_year == msg_time->tm_year && ltime_mon == msg_time->tm_mon && ltime_day < msg_time->tm_mday)) {
        MSG_TEXT *msg = calloc(1, sizeof(MSG_TEXT) + 256);
        time(&msg->time);
        msg->our_msg       = 0;
        msg->msg_type      = MSG_TYPE_NOTICE_DAY_CHANGE;
        msg->author_length = self.name_length;
        msg->length        = strftime((char *)msg->msg, 256, "Day has changed to %A %B %d %Y", msg_time);

        message_add(m, (MSG_VOID *)msg);
        return 1;
    }
    return 0;
}

/* TODO leaving this here is a little hacky, but it was the fastest way
 * without considering if I should expose messages_add */
uint32_t message_add_group(MESSAGES *m, MSG_TEXT *msg) { return message_add(m, (MSG_VOID *)msg); }

uint32_t message_add_type_text(MESSAGES *m, bool auth, const char *msgtxt, uint16_t length, bool log, bool send) {
    MSG_TEXT *msg = calloc(1, sizeof(MSG_TEXT) + length);
    time(&msg->time);
    msg->our_msg  = auth;
    msg->msg_type = MSG_TYPE_TEXT;
    msg->length   = length;

    if (auth) {
        msg->author_length = self.name_length;
        if (!send) {
            msg->receipt      = 0;
            msg->receipt_time = 1;
        }
    } else {
        msg->author_length = friend[m->id].name_length;
    }

    memcpy(msg->msg, msgtxt, length);

    if (m->data && m->number) {
        MSG_VOID *day_msg = m->data[m->number ? m->number - 1 : 0];
        msg_add_day_notice(m, day_msg->time, msg->time);
    }

    if (log) {
        message_log_to_disk(m, (MSG_VOID *)msg);
    }

    if (auth && send) {
        postmessage_toxcore(TOX_SEND_MESSAGE, friend[m->id].number, length, msg);
    }

    return message_add(m, (MSG_VOID *)msg);
}

uint32_t message_add_type_action(MESSAGES *m, bool auth, const char *msgtxt, uint16_t length, bool log, bool send) {
    MSG_TEXT *msg = calloc(1, sizeof(MSG_TEXT) + length);
    time(&msg->time);
    msg->our_msg  = auth;
    msg->msg_type = MSG_TYPE_ACTION_TEXT;
    msg->length   = length;

    if (auth) {
        msg->author_length = self.name_length;
        if (!send) {
            msg->receipt      = 0;
            msg->receipt_time = 1;
        }
    } else {
        msg->author_length = friend[m->id].name_length;
    }

    memcpy(msg->msg, msgtxt, length);

    if (log) {
        message_log_to_disk(m, (MSG_VOID *)msg);
    }

    if (auth && send) {
        postmessage_toxcore(TOX_SEND_ACTION, friend[m->id].number, length, msg);
    }

    return message_add(m, (MSG_VOID *)msg);
}

uint32_t message_add_type_notice(MESSAGES *m, const char *msgtxt, uint16_t length, bool log) {
    MSG_TEXT *msg = calloc(1, sizeof(MSG_TEXT) + length);
    time(&msg->time);
    msg->our_msg       = 0;
    msg->msg_type      = MSG_TYPE_NOTICE;
    msg->length        = length;
    msg->author_length = self.name_length;
    msg->receipt_time  = time(NULL);
    memcpy(msg->msg, msgtxt, length);

    if (log) {
        message_log_to_disk(m, (MSG_VOID *)msg);
    }

    return message_add(m, (MSG_VOID *)msg);
}

uint32_t message_add_type_image(MESSAGES *m, bool auth, NATIVE_IMAGE *img, uint16_t width, uint16_t height, bool log) {
    if (!NATIVE_IMAGE_IS_VALID(img)) {
        return 0;
    }

    MSG_IMG *msg = calloc(1, sizeof(MSG_IMG));
    time(&msg->time);
    msg->our_msg  = auth;
    msg->msg_type = MSG_TYPE_IMAGE;
    msg->w        = width;
    msg->h        = height;
    msg->zoom     = 0;
    msg->image    = img;
    msg->position = 0.0;

    return message_add(m, (MSG_VOID *)msg);
}

/* TODO FIX THIS SECTION TO MATCH ABOVE! */
/* Called by new file transfer to add a new message to the msg list */
MSG_FILE *message_add_type_file(MESSAGES *m, FILE_TRANSFER *file) {
    MSG_FILE *msg = calloc(1, sizeof(MSG_FILE));
    time(&msg->time);
    msg->our_msg     = file->incoming ? 0 : 1;
    msg->msg_type    = MSG_TYPE_FILE;
    msg->file_status = file->status;
    // msg->name_length is the max enforce that
    msg->name_length = (file->name_length > sizeof(msg->file_name)) ? sizeof(msg->file_name) : file->name_length;
    memcpy(msg->file_name, file->name, msg->name_length);
    msg->size       = file->size;
    msg->progress   = file->size_transferred;
    msg->speed      = 0;
    msg->inline_png = file->in_memory;
    msg->path       = NULL;

    msg->file = file;

    message_add(m, (MSG_VOID *)msg);

    file->ui_data = msg;

    return msg;
}

bool message_log_to_disk(MESSAGES *m, MSG_VOID *msg) {
    if (m->is_groupchat) {
        /* We don't support logging groupchats yet */
        return 0;
    }

    if (!settings.logging_enabled) {
        return 0;
    }

    FRIEND *f = &friend[m->id];
    if (f->skip_msg_logging) {
        return 0;
    }

    LOG_FILE_MSG_HEADER header;
    memset(&header, 0, sizeof(header));
    uint8_t *data = NULL;

    switch (msg->msg_type) {
        case MSG_TYPE_TEXT:
        case MSG_TYPE_ACTION_TEXT:
        case MSG_TYPE_NOTICE: {
            char *author;

            size_t    author_length;
            MSG_TEXT *text = (void *)msg;

            if (text->our_msg) {
                author_length = self.name_length;
                author        = self.name;
            } else {
                author_length = f->name_length;
                author        = f->name;
            }

            header.log_version   = LOGFILE_SAVE_VERSION;
            header.time          = text->time;
            header.author_length = author_length;
            header.msg_length    = text->length;
            header.author        = text->our_msg;
            header.receipt       = (text->receipt_time ? 1 : 0);
            header.msg_type      = text->msg_type;

            size_t length = sizeof(header) + text->length + author_length + 1; /* extra \n char*/

            data = calloc(1, length);
            memcpy(data, &header, sizeof(header));
            memcpy(data + sizeof(header), author, author_length);
            memcpy(data + sizeof(header) + author_length, text->msg, text->length);
            strcpy2(data + length - 1, "\n");

            msg->disk_offset = utox_save_chatlog(f->number, data, length);
            break;
        }
        default: { debug("uTox Logging:\tUnsupported file type %i\n", msg->msg_type); }
    }
    free(data);
    return 0;
}

bool messages_read_from_log(uint32_t friend_number) {
    size_t    actual_count = 0;
    uint8_t **data         = utox_load_chatlog(friend_number, &actual_count, UTOX_MAX_BACKLOG_MESSAGES, 0);
    MSG_VOID *msg;
    time_t    last = 0;

    if (data) {
        void **p = (void **)data;
        while (actual_count--) {
            msg = *p++;
            if (msg) {

                if (msg_add_day_notice(&friend[friend_number].msg, last, msg->time)) {
                    last = msg->time;
                }

                message_add(&friend[friend_number].msg, msg);
            }
        }
    } else {
        debug("If there's a friend history,there should be an error here...\n");
    }
    free(data);
    return 0;
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
            MSG_TEXT *msg = (MSG_TEXT *)(m->data[start]);
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
            MSG_TEXT *msg = (MSG_TEXT *)(m->data[start]);
            if (msg->msg_type == MSG_TYPE_TEXT || msg->msg_type == MSG_TYPE_ACTION_TEXT) {
                if (msg->our_msg && !msg->receipt_time) {
                    postmessage_toxcore((msg->msg_type == MSG_TYPE_TEXT ? TOX_SEND_MESSAGE : TOX_SEND_ACTION),
                                        friend_number, msg->length, msg);
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
            MSG_TEXT *msg = (MSG_TEXT *)m->data[start];
            if (msg->msg_type == MSG_TYPE_TEXT || msg->msg_type == MSG_TYPE_ACTION_TEXT) {
                if (msg->receipt == receipt_number) {

                    msg->receipt = -1;
                    time(&msg->receipt_time);

                    LOG_FILE_MSG_HEADER header;
                    memset(&header, 0, sizeof(header));
                    uint8_t *data = NULL;

                    header.log_version   = LOGFILE_SAVE_VERSION;
                    header.time          = msg->time;
                    header.author_length = msg->author_length;
                    header.msg_length    = msg->length;
                    header.author        = 1;
                    header.receipt       = 1;
                    header.msg_type      = msg->msg_type;

                    size_t length = sizeof(header);
                    data          = calloc(1, length);
                    memcpy(data, &header, sizeof(header));

                    if (msg->disk_offset) {
                        debug("Messages:\tUpdating message -> disk_offset is %lu\n", msg->disk_offset);
                        utox_update_chatlog(m->id, msg->disk_offset, data, length);
                    } else if (msg->disk_offset == 0 && start <= 1 && receipt_number == 1) {
                        /* This could get messy if receipt is 1 msg position is 0 and the offset is actually wrong,
                         * But I couldn't come up with any other way to verify the rare case of a bad offset
                         * start <= 1 to offset for the day change notification                                    */
                        debug("Messages:\tUpdating first message -> disk_offset is %lu\n", msg->disk_offset);
                        utox_update_chatlog(m->id, msg->disk_offset, data, length);
                    } else {
                        debug_error("Messages:\tUnable to update this message...\n"
                                    "\t\tmsg->disk_offset %lu && m->number %u receipt_number %u \n",
                                    msg->disk_offset, m->number, receipt_number);
                    }
                    free(data);

                    postmessage(FRIEND_MESSAGE, 0, 0, NULL); /* Used to redraw the screen */
                    pthread_mutex_unlock(&messages_lock);
                    return;
                }
            }
        }
    }
    debug_error("Messages:\tReceived a receipt for a message we don't have a record of. %u\n", receipt_number);
    pthread_mutex_unlock(&messages_lock);
}

static void messages_draw_timestamp(int x, int y, const time_t *time) {
    struct tm *ltime = localtime(time);

    char     timestr[9];
    uint16_t len;
    if (settings.use_long_time_msg) {
        len = snprintf(timestr, sizeof(timestr), "%.2u:%.2u:%.2u", ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
        x -= SCALE(8);
    } else {
        len = snprintf(timestr, sizeof(timestr), "%u:%.2u", ltime->tm_hour, ltime->tm_min);
    }

    if (len >= sizeof(timestr)) {
        len = sizeof(timestr) - 1;
    }

    setcolor(COLOR_MAIN_TEXT_SUBTEXT);
    setfont(FONT_MISC);
    drawtext(x, y, (char *)timestr, len);
}

static void messages_draw_author(int x, int y, int w, char *name, uint32_t length, uint32_t color) {
    setcolor(color);
    setfont(FONT_TEXT);
    drawtextwidth_right(x, w, y, name, length);
}

static int messages_draw_text(const char *msg, size_t length, uint32_t msg_height, uint8_t msg_type, bool author,
                              bool receipt, uint16_t highlight_start, uint16_t highlight_end, int x, int y, int w, int h) {
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
        debug("Text Draw Error:\ty %i | ny %i | mheight %u | width %i \n", y, ny, msg_height, w);
    }

    return ny;
}

/* draws an inline image at rect (x,y,width,height)
 *  maxwidth is maximum width the image can take in
 *  zoom is whether the image is currently zoomed in
 *  position is the y position along the image the player has scrolled */
static int messages_draw_image(MSG_IMG *img, int x, int y, int maxwidth) {
    image_set_filter(img->image, FILTER_BILINEAR);

    if (!img->zoom && img->w > maxwidth) {
        image_set_scale(img->image, (double)maxwidth / img->w);

        draw_image(img->image, x, y, maxwidth, img->h * maxwidth / img->w, 0, 0);

        image_set_scale(img->image, 1.0);
    } else {
        if (img->w > maxwidth) {
            draw_image(img->image, x, y, maxwidth, img->h, (int)((double)(img->w - maxwidth) * img->position), 0);
        } else {
            draw_image(img->image, x, y, img->w, img->h, 0, 0);
        }
    }

    return (img->zoom || img->w <= maxwidth) ? img->h : img->h * maxwidth / img->w;
}

static void messages_draw_filetransfer(MESSAGES *m, MSG_FILE *file, int i, int x, int y, int w, int h) {
    int room_for_clip = BM_FT_CAP_WIDTH + SCALE(2);
    int dx            = x + MESSAGES_X + room_for_clip;
    int d_width       = w - MESSAGES_X - TIME_WIDTH - room_for_clip;
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

    /* Get the values for the file transfer. */
    uint64_t file_size     = file->size;
    uint64_t file_progress = file->progress;
    uint64_t file_speed    = file->speed;
    uint64_t file_ttc      = file_speed ? (file_size - file_progress) / file_speed : 0;

    long double file_percent = (double)file_progress / (double)file_size;
    if (file_progress > file_size) {
        file_progress = file->size;
        file_percent  = 1.0;
    }

    char text_name_and_size[file->name_length + 33];
    memcpy(text_name_and_size, file->file_name, file->name_length);
    text_name_and_size[file->name_length] = ' ';
    uint16_t text_name_and_size_len       = file->name_length + 1;
    text_name_and_size_len += sprint_humanread_bytes(text_name_and_size + file->name_length + 1, 32, file_size);

    char     text_speed[32];
    uint16_t text_speed_len = sprint_humanread_bytes(text_speed, sizeof(text_speed), file_speed);
    if (text_speed_len <= 30) {
        text_speed[text_speed_len++] = '/';
        text_speed[text_speed_len++] = 's';
    }

    char     text_ttc[32];
    uint16_t text_ttc_len = snprintf(text_ttc, sizeof(text_ttc), "%lus", file_ttc);
    if (text_ttc_len >= sizeof(text_ttc)) {
        text_ttc_len = sizeof(text_ttc) - 1;
    }

    // progress rectangle
    uint32_t prog_bar = 0;

    setfont(FONT_MISC);
    setcolor(COLOR_BKGRND_MAIN);

/* Draw macros added, to reduce future line edits. */
#define draw_ft_rect(color) draw_rect_fill(dx, y, d_width, FILE_TRANSFER_BOX_HEIGHT, color)
#define draw_ft_prog(color) draw_rect_fill(dx, y, prog_bar, FILE_TRANSFER_BOX_HEIGHT, color)
#define draw_ft_cap(bg, fg)                                                                                 \
    do {                                                                                                    \
        drawalpha(BM_FT_CAP, dx - room_for_clip, y, BM_FT_CAP_WIDTH, BM_FTB_HEIGHT, bg);                    \
        drawalpha(BM_FILE, dx - room_for_clip + SCALE(4), y + SCALE(4), BM_FILE_WIDTH, BM_FILE_HEIGHT, fg); \
    } while (0)

/* Always first */
#define draw_ft_no_btn()                                                                        \
    do {                                                                                        \
        drawalpha(BM_FTB1, btnx, tbtn_bg_y, btn_bg_w, tbtn_bg_h,                                \
                  (mouse_left_btn ? COLOR_BTN_DANGER_BKGRND_HOVER : COLOR_BTN_SUCCESS_BKGRND)); \
        drawalpha(BM_NO, btnx + ((btn_bg_w - btnw) / 2), tbtn_y, btnw, btnh,                    \
                  (mouse_left_btn ? COLOR_BTN_DANGER_TEXT_HOVER : COLOR_BTN_DANGER_TEXT));      \
    } while (0)
/* Always last */
#define draw_ft_yes_btn()                                                                           \
    do {                                                                                            \
        drawalpha(BM_FTB2, btnx + btn_bg_w + SCALE(2), tbtn_bg_y, btn_bg_w, tbtn_bg_h,              \
                  (mouse_rght_btn ? COLOR_BTN_SUCCESS_BKGRND_HOVER : COLOR_BTN_SUCCESS_BKGRND));    \
        drawalpha(BM_YES, btnx + btn_bg_w + SCALE(2) + ((btn_bg_w - btnw) / 2), tbtn_y, btnw, btnh, \
                  (mouse_rght_btn ? COLOR_BTN_SUCCESS_TEXT_HOVER : COLOR_BTN_SUCCESS_TEXT));        \
    } while (0)
#define draw_ft_pause_btn()                                                                           \
    do {                                                                                              \
        drawalpha(BM_FTB2, btnx + btn_bg_w + SCALE(2), tbtn_bg_y, btn_bg_w, tbtn_bg_h,                \
                  (mouse_rght_btn ? COLOR_BTN_SUCCESS_BKGRND_HOVER : COLOR_BTN_SUCCESS_BKGRND));      \
        drawalpha(BM_PAUSE, btnx + btn_bg_w + SCALE(2) + ((btn_bg_w - btnw) / 2), tbtn_y, btnw, btnh, \
                  (mouse_rght_btn ? COLOR_BTN_SUCCESS_TEXT_HOVER : COLOR_BTN_SUCCESS_TEXT));          \
    } while (0)

#define draw_ft_resume_btn()                                                                           \
    do {                                                                                               \
        drawalpha(BM_FTB2, btnx + btn_bg_w + SCALE(2), tbtn_bg_y, btn_bg_w, tbtn_bg_h,                 \
                  (mouse_rght_btn ? COLOR_BTN_SUCCESS_BKGRND_HOVER : COLOR_BTN_SUCCESS_BKGRND));       \
        drawalpha(BM_RESUME, btnx + btn_bg_w + SCALE(2) + ((btn_bg_w - btnw) / 2), tbtn_y, btnw, btnh, \
                  (mouse_rght_btn ? COLOR_BTN_SUCCESS_TEXT_HOVER : COLOR_BTN_SUCCESS_TEXT));           \
    } while (0)

    int wbound = dx + d_width - SCALE(6);

#define draw_ft_text_right(str, len)                   \
    do {                                               \
        wbound -= (textwidth(str, len) + (SCALE(12))); \
        drawtext(wbound, y + SCALE(8), str, len);      \
    } while (0)
#define draw_ft_alph_right(bm, col)                     \
    do {                                                \
        wbound -= btnw + (SCALE(12));                   \
        drawalpha(bm, wbound, tbtn_y, btnw, btnh, col); \
    } while (0)
#define drawstr_ft_right(t) draw_ft_text_right(S(t), SLEN(t))

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
        default:
            // we'll round the corner even without buttons.
            d_width -= btn_bg_w;
            break;
    }

    prog_bar = (file->size == 0) ? 0 : ((long double)d_width * file_percent);

    switch (file->file_status) {
        case FILE_TRANSFER_STATUS_COMPLETED: {
            /* If mouse over use hover color */
            uint32_t text       = mouse_over ? COLOR_BTN_SUCCESS_TEXT_HOVER : COLOR_BTN_SUCCESS_TEXT,
                     background = mouse_over ? COLOR_BTN_SUCCESS_BKGRND_HOVER : COLOR_BTN_SUCCESS_BKGRND;

            setcolor(text);
            draw_ft_cap(background, text);
            draw_ft_rect(background);
            drawalpha(BM_FTB2, dx + d_width, tbtn_bg_y, btn_bg_w, tbtn_bg_h, background);

            if (file->inline_png) {
                drawstr_ft_right(CLICKTOSAVE);
            } else {
                drawstr_ft_right(CLICKTOOPEN);
            }
            draw_ft_alph_right(BM_YES, text);
            break;
        }
        case FILE_TRANSFER_STATUS_KILLED: {
            setcolor(COLOR_BTN_DANGER_TEXT);
            draw_ft_cap(COLOR_BTN_DANGER_BACKGROUND, COLOR_BTN_DANGER_TEXT);
            draw_ft_rect(COLOR_BTN_DANGER_BACKGROUND);
            drawalpha(BM_FTB2, dx + d_width, tbtn_bg_y, btn_bg_w, tbtn_bg_h, COLOR_BTN_DANGER_BACKGROUND);

            drawstr_ft_right(TRANSFER_CANCELLED);
            draw_ft_alph_right(BM_NO, COLOR_BTN_DANGER_TEXT);
            break;
        }
        case FILE_TRANSFER_STATUS_BROKEN: {
            setcolor(COLOR_BTN_DANGER_TEXT);
            draw_ft_cap(COLOR_BTN_DANGER_BACKGROUND, COLOR_BTN_DANGER_TEXT);
            draw_ft_rect(COLOR_BTN_DANGER_BACKGROUND);
            drawalpha(BM_FTB2, dx + d_width, tbtn_bg_y, btn_bg_w, tbtn_bg_h, COLOR_BTN_DANGER_BACKGROUND);

            drawstr_ft_right(TRANSFER_BROKEN);
            draw_ft_alph_right(BM_NO, COLOR_BTN_DANGER_TEXT);
            break;
        }
        case FILE_TRANSFER_STATUS_NONE: {
            /* ↑ used for incoming transfers */
            setcolor(COLOR_BTN_DISABLED_TRANSFER);
            draw_ft_cap(COLOR_BTN_DISABLED_BKGRND, COLOR_BTN_DISABLED_TRANSFER);
            draw_ft_rect(COLOR_BTN_DISABLED_BKGRND);

            draw_ft_no_btn();
            draw_ft_yes_btn();

            draw_ft_prog(COLOR_BTN_DISABLED_FORGRND);
            break;
        }
        case FILE_TRANSFER_STATUS_ACTIVE: {
            setcolor(COLOR_BTN_INPROGRESS_TEXT);
            draw_ft_cap(COLOR_BTN_INPROGRESS_BKGRND, COLOR_BTN_INPROGRESS_TEXT);
            draw_ft_rect(COLOR_BTN_INPROGRESS_BKGRND);

            draw_ft_no_btn();
            draw_ft_pause_btn();

            draw_ft_prog(COLOR_BTN_INPROGRESS_FORGRND);
            draw_ft_text_right(text_ttc, text_ttc_len);
            draw_ft_text_right(text_speed, text_speed_len);
            break;
        }
        case FILE_TRANSFER_STATUS_PAUSED_US:
        case FILE_TRANSFER_STATUS_PAUSED_BOTH:
        case FILE_TRANSFER_STATUS_PAUSED_THEM: {
            setcolor(COLOR_BTN_DISABLED_TRANSFER);

            draw_ft_cap(COLOR_BTN_DISABLED_BKGRND, COLOR_BTN_DISABLED_TRANSFER);
            draw_ft_rect(COLOR_BTN_DISABLED_BKGRND);

            draw_ft_no_btn();

            if (file->file_status == FILE_TRANSFER_STATUS_PAUSED_BOTH
                || file->file_status == FILE_TRANSFER_STATUS_PAUSED_US) {
                /* Paused by at least us */
                draw_ft_resume_btn();
            } else {
                /* Paused only by them */
                draw_ft_pause_btn();
            }

            draw_ft_prog(COLOR_BTN_DISABLED_FORGRND);
            break;
        }
    }

    setfont(FONT_TEXT);
    drawtextrange(dx + SCALE(10), wbound - SCALE(10), y + SCALE(6), text_name_and_size, text_name_and_size_len);
}

/* This is a bit hacky, and likely would benifit from being moved to a whole new section including seperating
 * group messages/functions from friend messages and functions from inside ui.c.
 *
 * Idealy group and friend messages wouldn't even need to know about eachother.   */
static int messages_draw_group(MESSAGES *m, MSG_GROUP *msg, uint32_t curr_msg_i, int x, int y, int width, int height) {
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
            h2 = msg->length;
        } else {
            h2 = m->sel_end_position;
        }
    }

    /* error check */
    if ((m->sel_start_msg == m->sel_end_msg && m->sel_start_position == m->sel_end_position) || h1 == h2) {
        h1 = UINT32_MAX;
        h2 = UINT32_MAX;
    }

    messages_draw_author(x, y, MESSAGES_X - NAME_OFFSET, msg->msg, msg->author_length, msg->author_color);
    messages_draw_timestamp(x + width - ACTUAL_TIME_WIDTH, y, &msg->time);
    return messages_draw_text(msg->msg + msg->author_length, msg->length, msg->height, msg->msg_type, msg->our_msg, 1,
                              h1, h2, x + MESSAGES_X, y, width - TIME_WIDTH - MESSAGES_X, height)
           + MESSAGES_SPACING;
}

/** Formats all messages from self and friends, and then call draw functions
 * to write them to the UI.
 *
 * accepts: messages struct *pointer, int x,y positions, int width,height
 */
void messages_draw(PANEL *panel, int x, int y, int width, int height) {
    pthread_mutex_lock(&messages_lock);

    MESSAGES *m = panel->object;
    // Do not draw author name next to every message
    uint8_t lastauthor = 0xFF;

    // Message iterator
    void **  p = m->data;
    uint32_t curr_msg_i, n = m->number;

    if (m->width != width) {
        m->width = width;
        messages_updateheight(m, width - MESSAGES_X + TIME_WIDTH);
        y -= scroll_gety(panel->content_scroll, height);
    }

    // Go through messages
    for (curr_msg_i = 0; curr_msg_i != n; curr_msg_i++) {
        MSG_TEXT *msg = *p++;

        /* Decide if we should even bother drawing this message. */
        if (msg->height == 0) {
            /* Empty message */
            pthread_mutex_unlock(&messages_lock);
            return;
        } else if (y + (int)msg->height <= MAIN_TOP) {
            /* message is exclusively above the viewing window */
            y += msg->height;
            continue;
        } else if (y >= height + SCALE(100)) { //! NOTE: should not be constant 100
            /* Message is exclusively below the viewing window */
            break;
        }

        // Draw the names for groups or friends
        if (m->is_groupchat) {
            y = messages_draw_group(m, (MSG_GROUP *)msg, curr_msg_i, x, y, width, height);
            continue;

        } else {
            FRIEND *f           = &friend[m->id];
            bool    draw_author = 1;
            if (msg->msg_type == MSG_TYPE_ACTION_TEXT) {
                // Always draw name next to action message
                lastauthor = 0xFF;
            } else if (msg->msg_type == MSG_TYPE_NOTICE_DAY_CHANGE) {
                draw_author = 0;
            }

            if (msg->msg_type == MSG_TYPE_NOTICE) {
                draw_author = 0;
            }

            if (draw_author) {
                if (msg->our_msg != lastauthor) {
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
                break;
            }

            case MSG_TYPE_TEXT:
            case MSG_TYPE_ACTION_TEXT:
            case MSG_TYPE_NOTICE: {
                // Draw timestamps
                messages_draw_timestamp(x + width - ACTUAL_TIME_WIDTH, y, &msg->time);
                /* intentional fall through */
            }
            case MSG_TYPE_NOTICE_DAY_CHANGE: {
                // Normal message
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
                        h2 = msg->length;
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
                if (h2 != msg->length) {
                    if (m->sel_end_msg != curr_msg_i) {
                        h2 = msg->length - h2;
                    } else {
                        h2 -= h1;
                    }
                }

                y = messages_draw_text(msg->msg, msg->length, msg->height, msg->msg_type, msg->our_msg, msg->receipt_time,
                                       h1, h2, x + MESSAGES_X, y, width - TIME_WIDTH - MESSAGES_X, height);
                break;
            }

            // Draw image
            case MSG_TYPE_IMAGE: {
                y += messages_draw_image((MSG_IMG *)msg, x + MESSAGES_X, y, width - MESSAGES_X - TIME_WIDTH);
                break;
            }

            // Draw file transfer
            case MSG_TYPE_FILE: {
                messages_draw_filetransfer(m, (MSG_FILE *)msg, curr_msg_i, x, y, width, height);
                y += FILE_TRANSFER_BOX_HEIGHT;
                break;
            }
        }

        y += MESSAGES_SPACING;
    }

    pthread_mutex_unlock(&messages_lock);
}

static bool messages_mmove_text(MESSAGES *m, int width, int mx, int my, int dy, char *message, uint32_t msg_height,
                                uint16_t msg_length) {

    cursor                  = CURSOR_TEXT;
    m->cursor_over_position = hittextmultiline(mx - MESSAGES_X, width - MESSAGES_X - TIME_WIDTH, (my < 0 ? 0 : my),
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
            if (m->cursor_over_uri == UINT32_MAX && end - str >= 7 && (strcmp2(str, "http://") == 0)) {
                cursor             = CURSOR_HAND;
                m->cursor_over_uri = str - message;
            } else if (m->cursor_over_uri == UINT32_MAX && end - str >= 8 && (strcmp2(str, "https://") == 0)) {
                cursor             = CURSOR_HAND;
                m->cursor_over_uri = str - message;
            }
        }
        str++;
    }

    if (m->cursor_over_uri != UINT32_MAX) {
        m->urllen          = (str - message) - m->cursor_over_uri;
        m->cursor_down_uri = prev_cursor_down_uri;
        debug("urllen %u\n", m->urllen);
    }

    return 0;
}

static bool messages_mmove_image(MSG_IMG *image, int max_width, int mx, int my) {
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

static uint8_t messages_mmove_filetransfer(MSG_FILE *file, int mx, int my, int width) {
    mx -= SCALE(10); /* Why? */
    if (mx >= 0 && mx < width && my >= 0 && my < FILE_TRANSFER_BOX_HEIGHT) {
        if (mx >= width - TIME_WIDTH - (BM_FTB_WIDTH * 2) - SCALE(2) - SCROLL_WIDTH
            && mx <= width - TIME_WIDTH - SCROLL_WIDTH) {
            if (mx >= width - TIME_WIDTH - BM_FTB_WIDTH - SCROLL_WIDTH) {
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
                    int UNUSED(dy)) {
    MESSAGES *m = panel->object;

    if (mx >= width - TIME_WIDTH) {
        m->cursor_over_time = 1;
    } else {
        m->cursor_over_time = 0;
    }

    if (m->cursor_down_msg < m->number) {
        int      maxwidth = width - MESSAGES_X - TIME_WIDTH;
        MSG_IMG *img_down = m->data[m->cursor_down_msg];
        if ((img_down->msg_type == MSG_TYPE_IMAGE) && (img_down->w > maxwidth)) {
            img_down->position -= (double)dx / (double)(img_down->w - maxwidth);
            if (img_down->position > 1.0) {
                img_down->position = 1.0;
            } else if (img_down->position < 0.0) {
                img_down->position = 0.0;
            }
            cursor = CURSOR_ZOOM_OUT;
            return 1;
        }
    }

    if (mx < 0 || my < 0 || (uint32_t)my > m->height) {
        if (m->cursor_over_msg != UINT32_MAX) {
            m->cursor_over_msg = UINT32_MAX;
            return 1;
        }
        return 0;
    }

    setfont(FONT_TEXT);

    void **  p = m->data;
    uint32_t i = 0, n = m->number;
    bool     need_redraw = 0;

    while (i < n) {
        MSG_TEXT *msg = *p++;

        int dy = msg->height; /* dy is the wrong name here, you should change it! */

        if (my >= 0 && my < dy) {
            m->cursor_over_msg = i;

            switch (msg->msg_type) {
                case MSG_TYPE_TEXT:
                case MSG_TYPE_ACTION_TEXT:
                case MSG_TYPE_NOTICE:
                case MSG_TYPE_NOTICE_DAY_CHANGE: {
                    if (m->is_groupchat) {
                        MSG_GROUP *grp = (void *)msg;
                        messages_mmove_text(m, width, mx, my, dy, grp->msg + grp->author_length, grp->height,
                                            grp->length);
                    } else {
                        messages_mmove_text(m, width, mx, my, dy, msg->msg, msg->height, msg->length);
                    }
                    if (m->cursor_down_msg != UINT32_MAX && (m->cursor_down_position != m->cursor_over_position
                                                             || m->cursor_down_msg != m->cursor_over_msg)) {
                        m->selecting_text = 1;
                    }
                    break;
                }

                case MSG_TYPE_IMAGE: {
                    m->cursor_over_position =
                        messages_mmove_image((MSG_IMG *)msg, (width - MESSAGES_X - TIME_WIDTH), mx, my);
                    break;
                }

                case MSG_TYPE_FILE: {
                    m->cursor_over_position = messages_mmove_filetransfer((MSG_FILE *)msg, mx, my, width);
                    if (m->cursor_over_position) {
                        need_redraw = 1;
                    }
                    break;
                }
            }

            if ((i != m->cursor_over_msg) && (m->cursor_over_msg != UINT32_MAX)
                && ((msg->msg_type == MSG_TYPE_FILE)
                    || (((MSG_FILE *)(m->data[m->cursor_over_msg]))->msg_type == MSG_TYPE_FILE))) {
                need_redraw = 1; // Redraw file on hover-in/out.
            }

            if (m->selecting_text) {
                need_redraw = 1;

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

    return 0;
}

bool messages_mdown(PANEL *panel) {
    MESSAGES *m        = panel->object;
    m->cursor_down_msg = UINT32_MAX;

    if (m->cursor_over_msg != UINT32_MAX) {

        MSG_VOID *msg = m->data[m->cursor_over_msg];
        switch (msg->msg_type) {
            case MSG_TYPE_TEXT:
            case MSG_TYPE_ACTION_TEXT:
            case MSG_TYPE_NOTICE:
            case MSG_TYPE_NOTICE_DAY_CHANGE: {
                if (m->cursor_over_uri != UINT32_MAX) {
                    m->cursor_down_uri = m->cursor_over_uri;
                    debug("mdn dURI %u, oURI %u\n", m->cursor_down_uri, m->cursor_over_uri);
                }

                m->sel_start_msg = m->sel_end_msg = m->cursor_down_msg = m->cursor_over_msg;
                m->sel_start_position = m->sel_end_position = m->cursor_down_position = m->cursor_over_position;
                break;
            }

            case MSG_TYPE_IMAGE: {
                MSG_IMG *img = (void *)msg;
                if (m->cursor_over_position) {
                    if (!img->zoom) {
                        img->zoom = 1;
                        message_updateheight(m, (MSG_VOID *)msg);
                    } else {
                        m->cursor_down_msg = m->cursor_over_msg;
                    }
                }
                break;
            }

            case MSG_TYPE_FILE: {
                MSG_FILE *file = (void *)msg;
                if (m->cursor_over_position == 0) {
                    break;
                }

                switch (file->file_status) {
                    case FILE_TRANSFER_STATUS_NONE: {
                        if (!msg->our_msg) {
                            if (m->cursor_over_position == 2) {
                                native_select_dir_ft(m->id, file);
                            } else if (m->cursor_over_position == 1) {
                                // decline
                                postmessage_toxcore(TOX_FILE_CANCEL, m->id, file->file->file_number, NULL);
                            }
                        } else if (m->cursor_over_position == 1) {
                            // cancel
                            postmessage_toxcore(TOX_FILE_CANCEL, m->id, file->file->file_number, NULL);
                        }

                        break;
                    }

                    case FILE_TRANSFER_STATUS_ACTIVE: {
                        if (m->cursor_over_position == 2) {
                            // pause
                            postmessage_toxcore(TOX_FILE_PAUSE, m->id, file->file->file_number, NULL);
                        } else if (m->cursor_over_position == 1) {
                            // cancel
                            postmessage_toxcore(TOX_FILE_CANCEL, m->id, file->file->file_number, NULL);
                        }
                        break;
                    }

                    case FILE_TRANSFER_STATUS_PAUSED_US: {
                        if (m->cursor_over_position == 2) {
                            // resume
                            postmessage_toxcore(TOX_FILE_RESUME, m->id, file->file->file_number, NULL);
                        } else if (m->cursor_over_position == 1) {
                            // cancel
                            postmessage_toxcore(TOX_FILE_CANCEL, m->id, file->file->file_number, NULL);
                        }
                        break;
                    }

                    case FILE_TRANSFER_STATUS_PAUSED_THEM:
                    case FILE_TRANSFER_STATUS_BROKEN: {
                        // cancel
                        if (m->cursor_over_position == 1) {
                            postmessage_toxcore(TOX_FILE_CANCEL, m->id, file->file->file_number, NULL);
                        }
                        break;
                    }

                    case FILE_TRANSFER_STATUS_COMPLETED: {
                        if (m->cursor_over_position) {
                            if (file->inline_png) {
                                savefiledata(file);
                            } else {
                                openurl(file->path);
                            }
                        }
                        break;
                    }
                }
                break;
            }
        }

        return 1;
    } else {
        if (m->sel_start_msg != m->sel_end_msg || m->sel_start_position != m->sel_end_position) {
            m->sel_start_msg      = 0;
            m->sel_end_msg        = 0;
            m->sel_start_position = 0;
            m->sel_end_position   = 0;
            return 1;
        }
    }

    return 0;
}

bool messages_dclick(PANEL *panel, bool triclick) {
    MESSAGES *m = panel->object;

    if (m->cursor_over_time) {
        settings.use_long_time_msg = !settings.use_long_time_msg;
        return 1;
    }

    if (m->cursor_over_msg != UINT32_MAX) {
        MSG_TEXT *msg      = m->data[m->cursor_over_msg];
        uint8_t * real_msg = NULL;

        if (m->is_groupchat) {
            real_msg = &((MSG_GROUP *)msg)->msg[((MSG_GROUP *)msg)->author_length]; /* This is hacky, we should
                                                                                   * probably fix this!      */
        } else {
            real_msg = msg->msg;
        }

        switch (msg->msg_type) {
            case MSG_TYPE_TEXT:
            case MSG_TYPE_ACTION_TEXT: {
                m->sel_start_msg = m->sel_end_msg = m->cursor_over_msg;

                char c = triclick ? '\n' : ' ';

                uint16_t i = m->cursor_over_position;
                while (i != 0 && real_msg[i - 1] != c) {
                    i -= utf8_unlen(real_msg + i);
                }
                m->sel_start_position = i;
                i                     = m->cursor_over_position;
                while (i != msg->length && real_msg[i] != c) {
                    i += utf8_len(real_msg + i);
                }
                m->sel_end_position = i;
                return 1;
            }
            case MSG_TYPE_IMAGE: {
                MSG_IMG *img = (void *)msg;
                if (m->cursor_over_position) {
                    if (img->zoom) {
                        img->zoom = 0;
                        message_updateheight(m, (MSG_VOID *)msg);
                    }
                }
                return 1;
            }
        }
    }
    return 0;
}

static void contextmenu_messages_onselect(uint8_t i) { copy(!!i); /* if not 0 force a 1 */ }

bool messages_mright(PANEL *panel) {
    MESSAGES *           m           = panel->object;
    static UTOX_I18N_STR menu_copy[] = { STR_COPY, STR_COPY_WITH_NAMES };
    if (m->cursor_over_msg == UINT32_MAX) {
        return 0;
    }

    MSG_TEXT *msg = m->data[m->cursor_over_msg];

    switch (msg->msg_type) {
        case MSG_TYPE_TEXT:
        case MSG_TYPE_ACTION_TEXT: {
            contextmenu_new(countof(menu_copy), menu_copy, contextmenu_messages_onselect);
            return 1;
        }
    }
    return 0;
}

bool messages_mwheel(PANEL *UNUSED(panel), int UNUSED(height), double UNUSED(d), bool UNUSED(smooth)) { return 0; }

bool messages_mup(PANEL *panel) {
    MESSAGES *m = panel->object;

    if (!m->data) {
        return 0;
    }

    if (m->cursor_over_msg != UINT32_MAX) {
        MSG_TEXT *msg = m->data[m->cursor_over_msg];
        if (msg->msg_type == MSG_TYPE_TEXT) {
            if (m->cursor_over_uri != UINT32_MAX && m->cursor_down_uri == m->cursor_over_uri
                && m->cursor_over_position >= m->cursor_over_uri
                && m->cursor_over_position <= m->cursor_over_uri + m->urllen - 1 /* - 1 Don't open on white space */
                && !m->selecting_text) {
                debug("mup dURI %u, oURI %u\n", m->cursor_down_uri, m->cursor_over_uri);
                char url[m->urllen + 1];
                memcpy(url, msg->msg + m->cursor_over_uri, m->urllen * sizeof(char));
                url[m->urllen] = 0;
                openurl(url);
                m->cursor_down_uri = 0;
            }
        }
    }

    // FIXME! temporary, change this
    /* lol... oh fuck... */
    if (m->selecting_text) {
        char *lel = malloc(65536); // TODO: De-hardcode this value.
        setselection(lel, messages_selection(panel, lel, 65536, 0));
        free(lel);

        m->selecting_text = 0;
    }

    m->cursor_down_msg = UINT32_MAX;

    return 0;
}

bool messages_mleave(PANEL *UNUSED(m)) { return 0; }

int messages_selection(PANEL *panel, void *buffer, uint32_t len, bool names) {
    MESSAGES *m = panel->object;

    if (m->number == 0) {
        *(char *)buffer = 0;
        return 0;
    }

    uint32_t i = m->sel_start_msg, n = m->sel_end_msg + 1;
    void **  dp = &m->data[i];

    char *p = buffer;

    while (i != UINT32_MAX && i != n) {
        MSG_TEXT *msg = *dp++;

        if (names && (i != m->sel_start_msg || m->sel_start_position == 0)) {
            if (m->is_groupchat) {
                MSG_GROUP *grp = (void *)msg;
                memcpy(p, &grp->msg[0], grp->author_length);
                p += grp->author_length;
                len -= grp->author_length;
            } else {
                FRIEND *f = &friend[m->id];

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
            case MSG_TYPE_TEXT:
            case MSG_TYPE_ACTION_TEXT: {
                uint16_t group_name_buffer = 0;
                if (m->is_groupchat) {
                    MSG_GROUP *grp    = (void *)msg;
                    group_name_buffer = grp->author_length;
                    group_name_buffer += 4; /* LOL, again, SO SORRY! This is about as hacky as it gets!
                                             * 2 is the number of bytes of difference between the
                                             * position of ->msg[0] of MSG_TEXT and of MSG_GROUP      */
                }

                char *   data;
                uint16_t length;
                if (i == m->sel_start_msg) {
                    if (i == m->sel_end_msg) {
                        data   = msg->msg + m->sel_start_position + group_name_buffer;
                        length = m->sel_end_position - m->sel_start_position;
                    } else {
                        data   = msg->msg + m->sel_start_position + group_name_buffer;
                        length = msg->length - m->sel_start_position;
                    }
                } else if (i == m->sel_end_msg) {
                    data   = msg->msg + group_name_buffer;
                    length = m->sel_end_position;
                } else {
                    data   = msg->msg + group_name_buffer;
                    length = msg->length;
                }

                if (len <= length) {
                    goto BREAK;
                }

                memcpy(p, data, length);
                p += length;
                len -= length;
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
BREAK:
    *p = 0;

    return (void *)p - buffer;
}

void messages_updateheight(MESSAGES *m, int width) {
    if (!m->data || !width) {
        return;
    }

    setfont(FONT_TEXT);

    uint32_t height = 0;
    uint32_t i      = 0;

    while (i < m->number) {
        height += message_setheight(m, (void *)m->data[i]);
        i++;
    }

    m->panel.content_scroll->content_height = m->height = height;
}

bool messages_char(uint32_t ch) {
    MESSAGES *m;
    if (flist_get_selected()->item == ITEM_FRIEND) {
        m = messages_friend.object;
    } else if (flist_get_selected()->item == ITEM_GROUP) {
        m = messages_group.object;
    } else {
        return 0;
    }

    switch (ch) {
        //! TODO: not constant 0.25
        /* TODO: probabaly need to fix this section :< m->panel.content scroll is likely to be wrong */
        case KEY_PAGEUP: {
            SCROLLABLE *scroll = m->panel.content_scroll;
            scroll->d -= 0.25;
            if (scroll->d < 0.0) {
                scroll->d = 0.0;
            }
            redraw();
            return 1;
        }

        case KEY_PAGEDOWN: {
            SCROLLABLE *scroll = m->panel.content_scroll;
            scroll->d += 0.25;
            if (scroll->d > 1.0) {
                scroll->d = 1.0;
            }
            redraw();
            return 1;
        }
    }

    return 0;
}

void messages_init(MESSAGES *m, uint32_t friend_number) {
    if (m->data) {
        messages_clear_all(m);
    }
    memset(m, 0, sizeof(*m) * countof(m));

    m->data = calloc(20, sizeof(void *));
    if (!m->data) {
        debug_error("\n\n\nFATAL ERROR TRYING TO CALLOC FOR MESSAGES.\nTHIS IS A BUG, PLEASE REPORT!\n\n\n");
        exit(30);
    }

    m->extra = 20;
    m->id    = friend_number;
}

void message_free(MSG_TEXT *msg) {
    switch (msg->msg_type) {
        case MSG_TYPE_IMAGE: {
            MSG_IMG *img = (void *)msg;
            image_free(img->image);
            break;
        }
        case MSG_TYPE_FILE: {
            // already gets free()d
            free(((MSG_FILE *)msg)->path);
            break;
        }
    }
    free(msg);
}

void messages_clear_all(MESSAGES *m) {
    pthread_mutex_lock(&messages_lock);
    uint32_t i;

    for (i = 0; i < m->number; i++) {
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
