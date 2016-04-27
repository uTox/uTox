#include "main.h"

static uint32_t message_add(MESSAGES *m, MSG_VOID *msg);

_Bool message_log_to_disk(MESSAGES *m, MSG_VOID *msg) {
    if (m->is_groupchat) {
        /* We don't support logging groupchats yet */
        return 0;
    }

    FRIEND *f = &friend[m->id];

    if (f->skip_msg_logging) {
        return 0;
    }

    LOG_FILE_MSG_HEADER header ;
    uint8_t *data = NULL;

    switch (msg->msg_type) {
        case MSG_TYPE_TEXT:
        case MSG_TYPE_ACTION_TEXT:
        case MSG_TYPE_NOTICE: {
            size_t author_length;
            uint8_t *author;
            MSG_TEXT *text = (void*)msg;

            if (text->author) {
                author_length = self.name_length;
                author = self.name;
            } else {
                author_length = f->name_length;
                author = f->name;
            }

            header.log_version   = 0;
            header.time          = text->time;
            header.author_length = author_length;
            header.msg_length    = text->length;
            header.author        = text->author;
            header.receipt       = (text->receipt_time ? 1 : 0);
            header.msg_type      = text->msg_type;


            size_t length = sizeof(header) + text->length + author_length + 1;

            data = calloc(1, length);
            memcpy(data, &header, sizeof(header));
            memcpy(data + sizeof(header), author, author_length);
            memcpy(data + sizeof(header) + author_length, text->msg, text->length);
            strcpy2(data + length - 1, "\n");

            native_save_data_log(f->number, data, length);
            break;
        }
        default: {
            debug("uTox Logging:\tUnsupported file type %i\n", msg->msg_type);
        }
    }
    free(data);
    return 0;
}

_Bool messages_read_from_log(uint32_t friend_number){
    size_t actual_count = 0;
    uint8_t **data = native_load_data_log(friend_number, &actual_count, UTOX_MAX_BACKLOG_MESSAGES, 0);
    MSG_VOID *msg;


    if (data) {
        void **p = (void**)data;
        while (actual_count--) {
            msg = *p++;
            if (msg) {
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
    uint32_t start = m->number;

    while (start--) {
        if (m->data[start]) {
            MSG_TEXT *msg = (MSG_TEXT*)(m->data[start]);
            if (msg->msg_type == MSG_TYPE_TEXT || msg->msg_type == MSG_TYPE_ACTION_TEXT) {
                if (!msg->receipt_time) {
                    postmessage_toxcore((msg->msg_type == MSG_TYPE_TEXT ? TOX_SEND_MESSAGE : TOX_SEND_ACTION),
                                        friend_number, msg->length, msg);
                }
            }
        }
    }
}

void messages_clear_receipt(MESSAGES *m, uint32_t receipt_number) {
    uint32_t start = m->number;

    while (start--) {
        if (m->data[start]) {
            MSG_TEXT *msg = (MSG_TEXT*)(m->data[start]);
            if (msg->msg_type == MSG_TYPE_TEXT || msg->msg_type == MSG_TYPE_ACTION_TEXT) {
                if (msg->receipt == receipt_number) {
                    msg->receipt = 0;
                    time(&msg->receipt_time);
                    return;
                }
            }
        }
    }

}

uint32_t message_add_type_text(MESSAGES *m, _Bool auth, const uint8_t *data, uint16_t length, _Bool log) {
    MSG_TEXT *msg   = calloc(1, sizeof(MSG_TEXT) + length);
    time(&msg->time);
    msg->author     = auth;
    msg->msg_type   = MSG_TYPE_TEXT;
    msg->length     = length;
    memcpy(msg->msg, data, length);

    if (log) {
        message_log_to_disk(m, (MSG_VOID*)msg);
    }

    if (auth) {
        postmessage_toxcore(TOX_SEND_MESSAGE, friend[m->id].number, length, msg);
    }

    return message_add(m, (MSG_VOID*)msg);
}

uint32_t message_add_type_action(MESSAGES *m, _Bool auth, const uint8_t *data, uint16_t length, _Bool log) {
    MSG_TEXT *msg   = calloc(1, sizeof(MSG_TEXT) + length);
    time(&msg->time);
    msg->author     = auth;
    msg->msg_type   = MSG_TYPE_ACTION_TEXT;
    msg->length     = length;
    memcpy(msg->msg, data, length);

    if (log) {
        message_log_to_disk(m, (MSG_VOID*)msg);
    }

    if (auth) {
        postmessage_toxcore(TOX_SEND_ACTION, friend[m->id].number, length, msg);
    }

    return message_add(m, (MSG_VOID*)msg);
}

uint32_t message_add_type_notice(MESSAGES *m, const uint8_t *data, uint16_t length, _Bool log) {
    MSG_TEXT *msg   = calloc(1, sizeof(MSG_TEXT) + length);
    time(&msg->time);
    msg->author     = 0;
    msg->msg_type   = MSG_TYPE_NOTICE;
    msg->length     = length;
    memcpy(msg->msg, data, length);

    if (log) {
        message_log_to_disk(m, (MSG_VOID*)msg);
    }

    return message_add(m, (MSG_VOID*)msg);
}

uint32_t message_add_type_image(MESSAGES *m, _Bool auth, UTOX_NATIVE_IMAGE *img, uint16_t width, uint16_t height, _Bool log) {
    if (!UTOX_NATIVE_IMAGE_IS_VALID(img)) {
        return 0;
    }

    MSG_IMG *msg = calloc(1, sizeof(MSG_IMG));
    time(&msg->time);
    msg->author = auth;
    msg->msg_type = MSG_TYPE_IMAGE;
    msg->w = width;
    msg->h = height;
    msg->zoom = 0;
    msg->image = img;
    msg->position = 0.0;

    return message_add(m, (MSG_VOID*)msg);
}

/* TODO FIX THIS SECTION TO MATCH ABOVE! */
/* Called by new file transfer to add a new message to the msg list */
MSG_FILE* message_create_type_file(FILE_TRANSFER *file) { //TODO shove on ui thread
    MSG_FILE *msg   = calloc(1, sizeof(MSG_FILE));
    time(&msg->time);
    msg->author     = file->incoming ? 0 : 1;
    msg->msg_type   = MSG_TYPE_FILE;
    msg->filenumber = file->file_number;
    msg->status     = file->status;
        // msg->name_length is the max enforce that
    msg->name_length = (file->name_length > sizeof(msg->name)) ? sizeof(msg->name) : file->name_length;
    memcpy(msg->name, file->name, msg->name_length);
    msg->size       = file->size;
    msg->progress   = file->size_transferred;
    msg->speed      = 0;
    msg->inline_png = file->in_memory;
    msg->path       = NULL;

    return msg;
}

// uint32_t message_add_type_file(MESSAGES *m, _Bool auth, file_number, status, name, name_length, local_path, local_length, file_size,) {}

uint32_t message_add_type_file_compat(MESSAGES *m, MSG_FILE *f) {
    return message_add(m, (MSG_VOID*)f);
}

static void messages_draw_timestamp(int x, int y, const time_t *time) {
    struct tm *ltime = localtime(time);
    char timestr[6];
    uint16_t len;
    len = snprintf(timestr, sizeof(timestr), "%u:%.2u", ltime->tm_hour, ltime->tm_min);

    if (len >= sizeof(timestr)) {
        len = sizeof(timestr) - 1;
    }

    setcolor(COLOR_MAIN_SUBTEXT);
    setfont(FONT_MISC);
    drawtext(x, y, (char_t*)timestr, len);
}

static void messages_draw_author(int x, int y, int w, uint8_t *name, uint32_t length, _Bool author) {
    if (author) {
        setcolor(COLOR_MAIN_SUBTEXT);
    } else {
        setcolor(COLOR_MAIN_CHATTEXT);
    }
    setfont(FONT_TEXT);
    drawtextwidth_right(x, w, y, name, length);
}

static int messages_draw_text(MSG_TEXT *msg, int x, int y, int w, int h, uint16_t h1, uint16_t h2) {

    switch (msg->msg_type) {
        case MSG_TYPE_TEXT: {
            if(msg->author) {
                if (msg->receipt_time) {
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
            setcolor(COLOR_MAIN_ACTIONTEXT);
            break;
        }
    }

    setfont(FONT_TEXT);

    int ny = utox_draw_text_multiline_within_box(x, y,
                                                 w + x, MAIN_TOP, y + msg->height,
                                                 font_small_lineheight,
                                                 msg->msg, msg->length,
                                                 h1, h2 - h1, 0, 0, 1);

    if (ny < y || (uint32_t)(ny - y) + MESSAGES_SPACING != msg->height) {
        debug("Text Draw Error:\ty %i | ny %i | mheight %u | width %i \n", y, ny, msg->height, w);
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
    int room_for_clip   = BM_FT_CAP_WIDTH + SCALE(2);
    int dx              = x + MESSAGES_X + room_for_clip;
    int d_width         = w - MESSAGES_X - TIME_WIDTH - room_for_clip;
    /* Mouse Positions */
    _Bool mo             = (m->cursor_over_msg == i);
    _Bool mouse_over     = (mo && m->cursor_over_position) ? 1 : 0;
    _Bool mouse_rght_btn = (mo && m->cursor_over_position == 2) ? 1 : 0;
    _Bool mouse_left_btn = (mo && m->cursor_over_position == 1) ? 1 : 0;

    /* Button Background */
    int btn_bg_w  = BM_FTB_WIDTH;
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
    uint64_t file_ttc      = file_speed ? (file_size - file_progress ) / file_speed : 0 ;

    long double file_percent  = (double)file_progress / (double)file_size;
    if (file_progress > file_size) {
        file_progress = file->size;
        file_percent = 1.0;
    }

    uint8_t text_name_and_size[file->name_length + 33];
    memcpy(text_name_and_size, file->name, file->name_length);
    text_name_and_size[file->name_length] = ' ';
    uint16_t text_name_and_size_len = file->name_length + 1;
    text_name_and_size_len += sprint_humanread_bytes(text_name_and_size + file->name_length + 1, 32, file_size);

    uint8_t  text_speed[32];
    uint16_t text_speed_len = sprint_humanread_bytes(text_speed, sizeof(text_speed), file_speed);
    if (text_speed_len <= 30) {
        text_speed[text_speed_len++] = '/';
        text_speed[text_speed_len++] = 's';
    }

    uint8_t text_ttc[32];
    uint16_t text_ttc_len = snprintf((char*)text_ttc, sizeof(text_ttc), "%lus", file_ttc);
    if (text_ttc_len >= sizeof(text_ttc)) {
        text_ttc_len = sizeof(text_ttc) - 1;
    }

    // progress rectangle
    uint32_t prog_bar = 0;

    setfont(FONT_MISC);
    setcolor(COLOR_BACKGROUND_MAIN);

    /* Draw macros added, to reduce future line edits. */
    #define draw_ft_rect(color) draw_rect_fill (dx, y, d_width, FILE_TRANSFER_BOX_HEIGHT, color)
    #define draw_ft_prog(color) draw_rect_fill (dx, y, prog_bar, FILE_TRANSFER_BOX_HEIGHT, color)
    #define draw_ft_cap(bg, fg) do { drawalpha(BM_FT_CAP, dx - room_for_clip, y, BM_FT_CAP_WIDTH, BM_FTB_HEIGHT, bg); \
                                     drawalpha(BM_FILE, dx - room_for_clip + SCALE(4), y + SCALE(4), BM_FILE_WIDTH, BM_FILE_HEIGHT, fg); } while (0)

    /* Always first */
    #define draw_ft_no_btn() do {   drawalpha(BM_FTB1, btnx, tbtn_bg_y, btn_bg_w, tbtn_bg_h,                          \
                                        (mouse_left_btn ? COLOR_BUTTON_DANGER_HOVER_BACKGROUND :                      \
                                                      COLOR_BUTTON_SUCCESS_BACKGROUND));                              \
                                    drawalpha(BM_NO, btnx + ((btn_bg_w - btnw) / 2), tbtn_y, btnw, btnh,              \
                                        (mouse_left_btn ? COLOR_BUTTON_DANGER_HOVER_TEXT :                            \
                                                      COLOR_BUTTON_DANGER_TEXT)); } while(0)
    /* Always last */
    #define draw_ft_yes_btn() do {  drawalpha(BM_FTB2, btnx + btn_bg_w + SCALE(2), tbtn_bg_y, btn_bg_w, tbtn_bg_h,    \
                                        (mouse_rght_btn ? COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND :                     \
                                                      COLOR_BUTTON_SUCCESS_BACKGROUND));                              \
                                    drawalpha(BM_YES, btnx + btn_bg_w + SCALE(2) + ((btn_bg_w - btnw) / 2),           \
                                        tbtn_y, btnw, btnh,                                                           \
                                        (mouse_rght_btn ? COLOR_BUTTON_SUCCESS_HOVER_TEXT :                           \
                                                      COLOR_BUTTON_SUCCESS_TEXT));} while(0)
    #define draw_ft_pause_btn() do {drawalpha(BM_FTB2, btnx + btn_bg_w + SCALE(2), tbtn_bg_y, btn_bg_w, tbtn_bg_h,    \
                                        (mouse_rght_btn ? COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND :                     \
                                                      COLOR_BUTTON_SUCCESS_BACKGROUND));                              \
                                    drawalpha(BM_PAUSE, btnx + btn_bg_w + SCALE(2) + ((btn_bg_w - btnw) / 2),         \
                                        tbtn_y, btnw, btnh,                                                           \
                                        (mouse_rght_btn ? COLOR_BUTTON_SUCCESS_HOVER_TEXT :                           \
                                                      COLOR_BUTTON_SUCCESS_TEXT));} while(0)

    #define draw_ft_resume_btn() do {drawalpha(BM_FTB2, btnx + btn_bg_w + SCALE(2), tbtn_bg_y, btn_bg_w, tbtn_bg_h,   \
                                        (mouse_rght_btn ? COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND :                     \
                                                      COLOR_BUTTON_SUCCESS_BACKGROUND));                              \
                                     drawalpha(BM_RESUME, btnx + btn_bg_w + SCALE(2) + ((btn_bg_w - btnw) / 2),       \
                                        tbtn_y, btnw, btnh,                                                           \
                                        (mouse_rght_btn ? COLOR_BUTTON_SUCCESS_HOVER_TEXT :                           \
                                                      COLOR_BUTTON_SUCCESS_TEXT));} while(0)

    int wbound = dx + d_width - SCALE(6);

    #define draw_ft_text_right(str, len) do { wbound -= (textwidth(str, len) + (SCALE(12))); drawtext(wbound, y + SCALE(8), str, len); } while (0)
    #define draw_ft_alph_right(bm, col) do { wbound -= btnw + (SCALE(12)); drawalpha(bm, wbound, tbtn_y, btnw, btnh, col); } while (0)
    #define drawstr_ft_right(t) draw_ft_text_right(S(t), SLEN(t))

    switch (file->status) {
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

    switch (file->status){
        case FILE_TRANSFER_STATUS_COMPLETED: {
            /* If mouse over use hover color */
            uint32_t text = mouse_over ? COLOR_BUTTON_SUCCESS_HOVER_TEXT : COLOR_BUTTON_SUCCESS_TEXT,
                     background = mouse_over ? COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND : COLOR_BUTTON_SUCCESS_BACKGROUND;

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
        case FILE_TRANSFER_STATUS_KILLED:{
            setcolor(COLOR_BUTTON_DANGER_TEXT);
            draw_ft_cap(COLOR_BUTTON_DANGER_BACKGROUND, COLOR_BUTTON_DANGER_TEXT);
            draw_ft_rect(COLOR_BUTTON_DANGER_BACKGROUND);
            drawalpha(BM_FTB2, dx + d_width, tbtn_bg_y, btn_bg_w, tbtn_bg_h, COLOR_BUTTON_DANGER_BACKGROUND);

            drawstr_ft_right(TRANSFER_CANCELLED);
            draw_ft_alph_right(BM_NO, COLOR_BUTTON_DANGER_TEXT);
            break;
        }
        case FILE_TRANSFER_STATUS_BROKEN: {
            setcolor(COLOR_BUTTON_DANGER_TEXT);
            draw_ft_cap(COLOR_BUTTON_DANGER_BACKGROUND, COLOR_BUTTON_DANGER_TEXT);
            draw_ft_rect(COLOR_BUTTON_DANGER_BACKGROUND);
            drawalpha(BM_FTB2, dx + d_width, tbtn_bg_y, btn_bg_w, tbtn_bg_h, COLOR_BUTTON_DANGER_BACKGROUND);

            drawstr_ft_right(TRANSFER_BROKEN);
            draw_ft_alph_right(BM_NO, COLOR_BUTTON_DANGER_TEXT);
            break;
        }
        case FILE_TRANSFER_STATUS_NONE: {
            /* ↑ used for incoming transfers */
            setcolor(COLOR_BUTTON_DISABLED_TRANSFER);
            draw_ft_cap(COLOR_BUTTON_DISABLED_BACKGROUND, COLOR_BUTTON_DISABLED_TRANSFER);
            draw_ft_rect(COLOR_BUTTON_DISABLED_BACKGROUND);

            draw_ft_no_btn();
            draw_ft_yes_btn();

            draw_ft_prog(COLOR_BUTTON_DISABLED_FOREGROUND);
            break;
        }
        case FILE_TRANSFER_STATUS_ACTIVE: {
            setcolor(COLOR_BUTTON_INPROGRESS_TEXT);
            draw_ft_cap(COLOR_BUTTON_INPROGRESS_BACKGROUND, COLOR_BUTTON_INPROGRESS_TEXT);
            draw_ft_rect(COLOR_BUTTON_INPROGRESS_BACKGROUND);

            draw_ft_no_btn();
            draw_ft_pause_btn();

            draw_ft_prog(COLOR_BUTTON_INPROGRESS_FOREGROUND);
            draw_ft_text_right(text_ttc, text_ttc_len);
            draw_ft_text_right(text_speed, text_speed_len);
            break;
        }
        case FILE_TRANSFER_STATUS_PAUSED_US:
        case FILE_TRANSFER_STATUS_PAUSED_BOTH:
        case FILE_TRANSFER_STATUS_PAUSED_THEM: {
            setcolor(COLOR_BUTTON_DISABLED_TRANSFER);

            draw_ft_cap(COLOR_BUTTON_DISABLED_BACKGROUND, COLOR_BUTTON_DISABLED_TRANSFER);
            draw_ft_rect(COLOR_BUTTON_DISABLED_BACKGROUND);

            draw_ft_no_btn();

            if (file->status == FILE_TRANSFER_STATUS_PAUSED_BOTH ||
                file->status == FILE_TRANSFER_STATUS_PAUSED_US    ) {
                /* Paused by at least us */
                draw_ft_resume_btn();
            } else {
                /* Paused only by them */
                draw_ft_pause_btn();
            }

            draw_ft_prog(COLOR_BUTTON_DISABLED_FOREGROUND);
            break;
        }
    }

    setfont(FONT_TEXT);
    drawtextrange(dx + SCALE(10), wbound - SCALE(10), y + SCALE(6), text_name_and_size, text_name_and_size_len);
}

/** Formats all messages from self and friends, and then call draw functions
 * to write them to the UI.
 *
 * accepts: messages struct *pointer, int x,y positions, int width,height
 */
void messages_draw(PANEL *panel, int x, int y, int width, int height) {
    MESSAGES *m = panel->object;
    // Do not draw author name next to every message
    uint8_t lastauthor = 0xFF;

    // Message iterator
    void **p = m->data;
    uint32_t i, n = m->number;

    if (m->width != width) {
        m->width = width;
        messages_updateheight(m, width - MESSAGES_X + TIME_WIDTH);
        y -= scroll_gety(panel->content_scroll, height);
    }

    // Go through messages
    for (i = 0; i != n; i++) {
        MSG_TEXT *msg = *p++;

        /* Decide if we should even bother drawing this message. */
        if (msg->height == 0) {
            /* Empty message */
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
            // Group message authors are all the same color
            messages_draw_author(x, y, MESSAGES_X - NAME_OFFSET,
                                 &msg->msg[msg->length] + 1,
                                 msg->msg[msg->length], 1);
        } else {
            FRIEND *f = &friend[m->id];
            _Bool auth = msg->author;
            _Bool draw_author = 1;
            if (msg->msg_type == MSG_TYPE_ACTION_TEXT) {
                // Always draw name next to action message
                lastauthor = 0xFF;
                auth = 1;
            }

            if (msg->msg_type == MSG_TYPE_NOTICE) {
                draw_author = 0;
            }

            if (draw_author) {
                if (msg->author != lastauthor) {
                    if (msg->author) {
                        messages_draw_author(x, y, MESSAGES_X - NAME_OFFSET, self.name, self.name_length, auth);
                    } else if (f->alias) {
                        messages_draw_author(x, y, MESSAGES_X - NAME_OFFSET, f->alias, f->alias_length, auth);
                    } else {
                        messages_draw_author(x, y, MESSAGES_X - NAME_OFFSET, f->name, f->name_length, auth);
                    }
                    lastauthor = msg->author;
                }
            }
        }

        // Draw message contents
        switch(msg->msg_type) {
            case MSG_TYPE_NULL: {
                break;
            }

            case MSG_TYPE_TEXT:
            case MSG_TYPE_ACTION_TEXT: {
                // Draw timestamps
                messages_draw_timestamp(x + width - ACTUAL_TIME_WIDTH, y, &msg->time);
                /* intentional fall through */
            }
            case MSG_TYPE_NOTICE:
            case MSG_TYPE_NOTICE_DAY_CHANGE: {
                // Normal message
                uint16_t h1 = UINT16_MAX, h2 = UINT16_MAX;
                if (i == m->sel_start_msg) {
                    h1 = m->sel_start_position;
                    h2 = ((i == m->sel_end_msg) ? m->sel_end_position : msg->length);
                } else if (i == m->sel_end_msg) {
                    h1 = 0;
                    h2 = m->sel_end_position;
                } else if (i > m->sel_start_msg && i < m->sel_end_msg) {
                    h1 = 0;
                    h2 = msg->length;
                }

                if ((m->sel_start_msg == m->sel_end_msg && m->sel_start_position == m->sel_end_position) || h1 == h2) {
                    h1 = UINT16_MAX;
                    h2 = UINT16_MAX;
                }

                y = messages_draw_text(msg, x + MESSAGES_X, y, width - TIME_WIDTH - MESSAGES_X, height, h1, h2);
                break;
            }

            // Draw image
            case MSG_TYPE_IMAGE: {
                y += messages_draw_image((MSG_IMG*)msg, x + MESSAGES_X, y, width - MESSAGES_X - TIME_WIDTH);
                break;
            }

            // Draw file transfer
            case MSG_TYPE_FILE: {
                messages_draw_filetransfer(m, (MSG_FILE*)msg, i, x, y, width, height);
                y += FILE_TRANSFER_BOX_HEIGHT;
                break;
            }
        }

        y += MESSAGES_SPACING;
    }
}

static _Bool messages_mmove_text(MESSAGES *m, int width, int mx, int my, int dy,
                                 char_t *message, uint32_t msg_height, uint16_t msg_length)
{

    cursor = CURSOR_TEXT;
    m->cursor_over_position = hittextmultiline(mx - MESSAGES_X,
                               width - MESSAGES_X - TIME_WIDTH,
                               (my < 0 ? 0 : my),
                               msg_height,
                               font_small_lineheight,
                               message,
                               msg_length,
                               1);

    if (my < 0 || my >= dy || mx < MESSAGES_X || m->cursor_over_position == msg_length) {
        return 0;
    }

    _Bool prev_mouse_down_on_uri = m->mouse_down_on_uri;

    if (m->mouse_over_uri != UINT16_MAX) {
        m->mouse_down_on_uri = 0;
        m->mouse_over_uri = UINT16_MAX;
    }


    char_t *str = message + m->cursor_over_position;
    while (str != message) {
        str--;
        if (*str == ' ' || *str == '\n') {
            str++;
            break;
        }
    }

    char_t *end = message + msg_length;
    while (str != end && *str != ' ' && *str != '\n') {
        if (str == message || *(str - 1) == '\n' || *(str - 1) == ' ') {
            if ((m->mouse_over_uri == UINT16_MAX && end - str >= 7 && strcmp2(str, "http://") == 0)) {
                cursor = CURSOR_HAND;
                m->mouse_over_uri = str - message;
            } else if ((m->mouse_over_uri == UINT16_MAX && end - str >= 8 && strcmp2(str, "https://") == 0)) {
                cursor = CURSOR_HAND;
                m->mouse_over_uri = str - message;
            }
        }
        str++;
    }

    if (m->mouse_over_uri != UINT16_MAX) {
        m->urllen = (str - message) - m->mouse_over_uri;
        m->mouse_down_on_uri = prev_mouse_down_on_uri;
    }
    return 0;
}

static _Bool messages_mmove_image(MSG_IMG *image, int max_width, int mx, int my){
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

_Bool messages_mmove(PANEL *panel, int UNUSED(px), int UNUSED(py), int width, int UNUSED(height),
                     int mx, int my, int dx, int UNUSED(dy))
{
    MESSAGES *m = panel->object;
    if (m->cursor_down_msg < m->number) {
        int maxwidth = width - MESSAGES_X - TIME_WIDTH;
        MSG_IMG *img_down = m->data[m->cursor_down_msg];
        if((img_down->msg_type == MSG_TYPE_IMAGE) && (img_down->w > maxwidth)) {
            img_down->position -= (double)dx / (double)(img_down->w - maxwidth);
            if(img_down->position > 1.0) {
                img_down->position = 1.0;
            } else if(img_down->position < 0.0) {
                img_down->position = 0.0;
            }
            cursor = CURSOR_ZOOM_OUT;
            return 1;
        }
    }

    if (mx < 0 || my < 0 || (uint32_t) my > m->height) {
        if(m->cursor_over_msg != UINT32_MAX) {
            m->cursor_over_msg = UINT32_MAX;
            return 1;
        }
        return 0;
    }

    setfont(FONT_TEXT);

    void **p = m->data;
    uint32_t i = 0, n = m->number;
    _Bool need_redraw = 0;

    while (i < n) {
        MSG_TEXT *msg = *p++;

        int dy = msg->height;

        if (my >= 0 && my < dy) {
            m->cursor_over_msg = i;

            switch(msg->msg_type) {
                case MSG_TYPE_TEXT:
                case MSG_TYPE_ACTION_TEXT:
                case MSG_TYPE_NOTICE:
                case MSG_TYPE_NOTICE_DAY_CHANGE: {
                    messages_mmove_text(m, width, mx, my, dy, msg->msg, msg->height, msg->length);
                    break;
                }

                case MSG_TYPE_IMAGE: {
                    m->cursor_over_position = messages_mmove_image((MSG_IMG*)msg, (width - MESSAGES_X - TIME_WIDTH), mx, my);
                    break;
                }

                case MSG_TYPE_FILE: {
                    m->cursor_over_position = messages_mmove_filetransfer((MSG_FILE*)msg, mx, my, width);
                    if (m->cursor_over_position) {
                        need_redraw = 1;
                    }
                    break;
                }
            }

            if ((i != m->cursor_over_msg)
                && (m->cursor_over_msg != UINT32_MAX)
                && ((msg->msg_type == MSG_TYPE_FILE)
                    || (((MSG_FILE*)(m->data[m->cursor_over_msg]))->msg_type == MSG_TYPE_FILE))) {
                need_redraw = 1; // Redraw file on hover-in/out.
            }


            if (m->selecting_text) {
                uint32_t msg_start, msg_end;
                uint16_t pos_start, pos_end;
                if (i > m->cursor_down_msg) {
                    msg_start = m->cursor_down_msg;
                    msg_end = i;

                    pos_start = m->cursor_down_position;
                    pos_end = m->cursor_over_position;
                } else if (i < m->cursor_down_msg) {
                    msg_end = m->cursor_down_msg;
                    msg_start = i;

                    pos_end = m->cursor_down_position;
                    pos_start = m->cursor_over_position;
                } else {
                    msg_start = msg_end = i;
                    if (m->cursor_over_position >= m->cursor_down_position) {
                        pos_start = m->cursor_down_position;
                        pos_end = m->cursor_over_position;
                    } else {
                        pos_end = m->cursor_down_position;
                        pos_start = m->cursor_over_position;
                    }
                }

                if (  pos_start != m->sel_start_position
                   || msg_start != m->sel_start_msg
                   ||   pos_end != m->sel_end_position
                   ||   msg_end != m->sel_end_msg) {

                    m->sel_start_position   = pos_start;
                    m->sel_end_position     = pos_end;
                    m->sel_start_msg        = msg_start;
                    m->sel_end_msg          = msg_end;
                    need_redraw = 1;
                }
            }

            return need_redraw;
        }

        my -= dy;

        i++;
    }

    return 0;
}

_Bool messages_mdown(PANEL *panel) {
    MESSAGES *m = panel->object;
    m->cursor_down_msg = UINT32_MAX;

    if (m->cursor_over_msg != UINT32_MAX) {

        MSG_VOID *msg = m->data[m->cursor_over_msg];
        switch (msg->msg_type) {
            case MSG_TYPE_TEXT:
            case MSG_TYPE_ACTION_TEXT:
            case MSG_TYPE_NOTICE:
            case MSG_TYPE_NOTICE_DAY_CHANGE: {
                if (m->mouse_over_uri != UINT16_MAX) {
                    m->mouse_down_on_uri = 1;
                }

                m->sel_start_msg = m->sel_end_msg = m->cursor_down_msg = m->cursor_over_msg;
                m->sel_start_position = m->sel_end_position = m->cursor_down_position = m->cursor_over_position;
                m->selecting_text = 1;
                break;
            }

            case MSG_TYPE_IMAGE: {
                MSG_IMG *img = (void*)msg;
                if (m->cursor_over_position) {
                    if(!img->zoom) {
                        img->zoom = 1;
                        message_updateheight(m, (MSG_VOID*)msg);
                    } else {
                        m->cursor_down_msg = m->cursor_over_msg;
                    }
                }
                break;
            }

            case MSG_TYPE_FILE: {
                MSG_FILE *file = (void*)msg;
                if(m->cursor_over_position == 0) {
                    break;
                }

                switch(file->status) {
                case FILE_TRANSFER_STATUS_NONE: {
                    if(!msg->author) {
                        if(m->cursor_over_position == 2) {
                            native_select_dir_ft(m->id, file);
                        } else if(m->cursor_over_position == 1) {
                            //decline
                            postmessage_toxcore(TOX_FILE_CANCEL, m->id, file->filenumber, NULL);
                        }
                    } else if(m->cursor_over_position == 1) {
                        //cancel
                        postmessage_toxcore(TOX_FILE_CANCEL, m->id, file->filenumber, NULL);
                    }


                    break;
                }

                case FILE_TRANSFER_STATUS_ACTIVE: {
                    if(m->cursor_over_position == 2) {
                        //pause
                        postmessage_toxcore(TOX_FILE_PAUSE, m->id, file->filenumber, NULL);
                    } else if(m->cursor_over_position == 1) {
                        //cancel
                        postmessage_toxcore(TOX_FILE_CANCEL, m->id, file->filenumber, NULL);
                    }
                    break;
                }

                case FILE_TRANSFER_STATUS_PAUSED_US: {
                    if(m->cursor_over_position == 2) {
                        //resume
                        postmessage_toxcore(TOX_FILE_RESUME, m->id, file->filenumber, NULL);
                    } else if(m->cursor_over_position == 1) {
                        //cancel
                        postmessage_toxcore(TOX_FILE_CANCEL, m->id, file->filenumber, NULL);
                    }
                    break;
                }

                case FILE_TRANSFER_STATUS_PAUSED_THEM:
                case FILE_TRANSFER_STATUS_BROKEN: {
                    //cancel
                    if(m->cursor_over_position == 1) {
                        postmessage_toxcore(TOX_FILE_CANCEL, m->id, file->filenumber, NULL);
                    }
                    break;
                }

                case FILE_TRANSFER_STATUS_COMPLETED: {
                    if(m->cursor_over_position) {
                        if(file->inline_png) {
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
        if(m->sel_start_msg != m->sel_end_msg || m->sel_start_position != m->sel_end_position) {
            m->sel_start_msg = 0;
            m->sel_end_msg = 0;
            m->sel_start_position = 0;
            m->sel_end_position = 0;
            return 1;
        }
    }

    return 0;
}

_Bool messages_dclick(PANEL *panel, _Bool triclick) {
    MESSAGES *m = panel->object;

    if(m->cursor_over_msg != UINT32_MAX) {
        MSG_TEXT *msg = m->data[m->cursor_over_msg];
        switch(msg->msg_type) {
            case MSG_TYPE_TEXT:
            case MSG_TYPE_ACTION_TEXT: {
                m->sel_start_msg = m->sel_end_msg = m->cursor_over_msg;

                char_t c = triclick ? '\n' : ' ';

                uint16_t i = m->cursor_over_position;
                while (i != 0 && msg->msg[i - 1] != c) {
                    i -= utf8_unlen(msg->msg + i);
                }
                m->sel_start_position = i;
                i = m->cursor_over_position;
                while (i != msg->length && msg->msg[i] != c) {
                    i += utf8_len(msg->msg + i);
                }
                m->sel_end_position = i;
                return 1;
            }
            case MSG_TYPE_IMAGE: {
                MSG_IMG *img = (void*)msg;
                if (m->cursor_over_position) {
                    if (img->zoom) {
                        img->zoom = 0;
                        message_updateheight(m, (MSG_VOID*)msg);
                    }
                }
                return 1;
            }
        }
    }
    return 0;
}

static void contextmenu_messages_onselect(uint8_t i){
    copy(!!i); /* if not 0 force a 1 */
}

_Bool messages_mright(PANEL *panel) {
    MESSAGES *m = panel->object;
    static UI_STRING_ID menu_copy[] = {STR_COPY, STR_COPY_WITH_NAMES};
    if(m->cursor_over_msg == UINT32_MAX) {
        return 0;
    }

    MSG_TEXT* msg = m->data[m->cursor_over_msg];

    switch(msg->msg_type) {
        case MSG_TYPE_TEXT:
        case MSG_TYPE_ACTION_TEXT: {
            contextmenu_new(countof(menu_copy), menu_copy, contextmenu_messages_onselect);
            return 1;
        }
    }
    return 0;
}

_Bool messages_mwheel(PANEL *UNUSED(panel), int UNUSED(height), double UNUSED(d), _Bool UNUSED(smooth)) {
    return 0;
}

_Bool messages_mup(PANEL *panel) {
    MESSAGES *m = panel->object;

    if (!m->data) {
        return 0;
    }

    if (m->cursor_over_msg != UINT32_MAX) {

        MSG_TEXT *msg = m->data[m->cursor_over_msg];
        if (msg->msg_type == MSG_TYPE_TEXT){
            if (m->mouse_over_uri != UINT16_MAX && m->mouse_down_on_uri) {
                char_t url[m->urllen + 1];
                memcpy(url, msg->msg + m->mouse_over_uri, m->urllen * sizeof(char_t));
                url[m->urllen] = 0;
                openurl(url);
                m->mouse_down_on_uri = 0;
            }
        }
    }

    //temporary, change this
    /* lol... oh fuck... */
    if (m->selecting_text) {
        char_t *lel = malloc(65536); //TODO: De-hardcode this value.
        setselection(lel, messages_selection(panel, lel, 65536, 0));
        free(lel);

        m->selecting_text = 0;
    }

    m->cursor_down_msg = UINT32_MAX;

    return 0;
}

_Bool messages_mleave(PANEL *UNUSED(m))
{
    return 0;
}

int messages_selection(PANEL *panel, void *buffer, uint32_t len, _Bool names) {
    MESSAGES* m = panel->object;

    if (m->number == 0) {
        *(char_t*)buffer = 0;
        return 0;
    }

    uint32_t i = m->sel_start_msg, n = m->sel_end_msg + 1;
    void **dp = &m->data[i];

    char_t *p = buffer;

    while (i != UINT32_MAX && i != n) {
        MSG_TEXT *msg = *dp++;

        if (names && (i != m->sel_start_msg || m->sel_start_position == 0)) {
            if (m->is_groupchat) {
                //TODO: get rid of such hacks or provide unpacker.
                //This basically undoes copy_groupmessage().
                uint8_t l = (uint8_t)msg->msg[msg->length];
                if (len <= l) {
                    break;
                }

                memcpy(p, &msg->msg[msg->length + 1], l);
                p += l;
                len -= l;
            } else {
                FRIEND *f = &friend[m->id];

                if (!msg->author) {
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

        switch(msg->msg_type) {
            case MSG_TYPE_TEXT:
            case MSG_TYPE_ACTION_TEXT: {
                char_t *data;
                uint16_t length;
                if(i == m->sel_start_msg) {
                    if(i == m->sel_end_msg) {
                        data = msg->msg + m->sel_start_position;
                        length = m->sel_end_position - m->sel_start_position;
                    } else {
                        data = msg->msg + m->sel_start_position;
                        length = msg->length - m->sel_start_position;
                    }
                } else if(i == m->sel_end_msg) {
                    data = msg->msg;
                    length = m->sel_end_position;
                } else {
                    data = msg->msg;
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

        if(i != n) {
            #ifdef __WIN32__
            if(len <= 2) {
                break;
            }
            *p++ = '\r';
            *p++ = '\n';
            len -= 2;
            #else
            if(len <= 1) {
                break;
            }
            *p++ = '\n';
            len--;
            #endif
        }
    }
    BREAK:
    *p = 0;

    return (void*)p - buffer;
}

static int msgheight(MSG_VOID *msg, int width) {
    switch(msg->msg_type) {
        case MSG_TYPE_TEXT:
        case MSG_TYPE_ACTION_TEXT:
        case MSG_TYPE_NOTICE:
        case MSG_TYPE_NOTICE_DAY_CHANGE: {
            MSG_TEXT *text = (void*)msg;
            int theight = text_height(abs(width - MESSAGES_X - TIME_WIDTH), font_small_lineheight, text->msg, text->length);
            return (theight == 0) ? 0 : theight + MESSAGES_SPACING;
        }

        case MSG_TYPE_IMAGE: {
            MSG_IMG *img = (void*)msg;
            int maxwidth = width - MESSAGES_X - TIME_WIDTH;
            return ((img->zoom || img->w <= maxwidth) ? img->h : img->h * maxwidth / img->w) + MESSAGES_SPACING;
        }

        case MSG_TYPE_FILE: {
            return FILE_TRANSFER_BOX_HEIGHT + MESSAGES_SPACING;
        }

    }

    return 0;
}

void messages_updateheight(MESSAGES *m, int width) {
    if (!m->data || !width) {
        return;
    }

    setfont(FONT_TEXT);

    uint32_t height = 0;
    uint32_t i = 0;

    while (i < m->number) {
        MSG_VOID *msg = m->data[i];
        msg->height   = msgheight(msg, m->width);
        height       += msg->height;
        i++;
    }

    m->panel.content_scroll->content_height = m->height = height;
}

static void message_setheight(MESSAGES *m, MSG_VOID *msg) {

    if (m->width == 0) {
        return;
    }

    setfont(FONT_TEXT);

    msg->height  = msgheight((MSG_VOID*)msg, m->width);
    m->height   += msg->height;
    m->panel.content_scroll->content_height = m->height;
}

void message_updateheight(MESSAGES *m, MSG_VOID *msg) {
    if (m->width == 0) {
        return;
    }

    setfont(FONT_TEXT);

    m->height   -= msg->height;
    msg->height  = msgheight(msg, m->width);
    m->height   += msg->height;

    m->panel.content_scroll->content_height = m->height;
}

/** Appends a messages from self or friend to the message list;
 * will realloc or trim messages as needed;
 *
 * also handles auto scrolling selections with messages
 *
 * accepts: MESSAGES *pointer, MESSAGE *pointer, MSG_DATA *pointer
 */
static uint32_t message_add(MESSAGES *m, MSG_VOID *msg) {
    /* TODO: test this? */
    if (m->number < UTOX_MAX_BACKLOG_MESSAGES) {
        if (m->extra <= 0) {
            if (m->data) {
                m->data = realloc(m->data, (m->number + 10) * sizeof(void*));
                m->extra += 10;
            } else {
                m->data = calloc(20, sizeof(void*));
                m->extra = 20;
            }

            if (!m->data) {
                debug("\n\n\nFATIAL ERROR TRYING TO REALLOC FOR MESSAGES.\nTHIS IS A BUG, PLEASE REPORT!\n\n\n");
                exit(30);
            }
        }
        m->data[m->number++] = msg;
        m->extra--;
    } else {
        m->height -= ((MSG_VOID*)m->data[0])->height;
        /* Assuming this is MSG_TEXT is probably a mistake... */
        message_free(m->data[0]);
        memmove(m->data, m->data + 1, (UTOX_MAX_BACKLOG_MESSAGES - 1) * sizeof(void*));
        m->data[UTOX_MAX_BACKLOG_MESSAGES - 1] = msg;

        // Scroll selection up so that it stays over the same messages.
        if (m->sel_start_msg != UINT32_MAX) {
            if(0 < m->sel_start_msg) {
                m->sel_start_msg--;
            } else {
                m->sel_start_position = 0;
            }
        }

        if (m->sel_end_msg != UINT32_MAX) {
            if(0 < m->sel_end_msg) {
                m->sel_end_msg--;
            } else {
                m->sel_end_position = 0;
            }
        }

        if (m->cursor_down_msg != UINT32_MAX) {
            if(0 < m->cursor_down_msg) {
                m->cursor_down_msg--;
            } else {
                m->cursor_down_position = 0;
            }
        }
        if (m->cursor_over_msg != UINT32_MAX) {
            if(0 < m->cursor_over_msg) {
                m->cursor_over_msg--;
            } else {
                m->cursor_over_position = 0;
            }
        }
    }

    message_setheight(m, (MSG_VOID*)msg);

    return m->number;
}

_Bool messages_char(uint32_t ch) {
    MESSAGES *m;
    if(selected_item->item == ITEM_FRIEND) {
        m = messages_friend.object;
    } else if(selected_item->item == ITEM_GROUP) {
        m = messages_group.object;
    } else {
        return 0;
    }

    switch(ch) {
        //!TODO: not constant 0.25
        /* TODO: probabaly need to fix this section :< m->panel.content scroll is likely to be wrong */
        case KEY_PAGEUP: {
            SCROLLABLE *scroll = m->panel.content_scroll;
            scroll->d -= 0.25;
            if(scroll->d < 0.0) {
                scroll->d = 0.0;
            }
            redraw();
            return 1;
        }

        case KEY_PAGEDOWN: {
            SCROLLABLE *scroll = m->panel.content_scroll;
            scroll->d += 0.25;
            if(scroll->d > 1.0) {
                scroll->d = 1.0;
            }
            redraw();
            return 1;
        }
    }

    return 0;
}

void message_free(MSG_TEXT *msg) {
    switch(msg->msg_type) {
        case MSG_TYPE_IMAGE: {
            MSG_IMG *img = (void*)msg;
            image_free(img->image);
            break;
        }
        case MSG_TYPE_FILE: {
            //already gets free()d
            free(((MSG_FILE*)msg)->path);
            break;
        }
    }
    free(msg);
}

void messages_clear_all(MESSAGES *m) {
    uint32_t i;

    for (i = 0; i < m->number; i++) {
        message_free(m->data[i]);
    }

    free(m->data);
    m->data = NULL;
    m->number = 0;

    m->sel_start_msg = m->sel_end_msg = m->sel_start_position = m->sel_end_position = 0;

    m->panel.content_scroll->content_height = m->height = 0;
}
