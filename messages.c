#include "main.h"

/* draws an inline image at rect (x,y,width,height)
 *  maxwidth is maximum width the image can take in
 *  zoom is whether the image is currently zoomed in
 *  position is the y position along the image the player has scrolled */
static void draw_message_image(UTOX_NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height,
                               uint32_t maxwidth, _Bool zoom, double position) {
    image_set_filter(image, FILTER_BILINEAR);

    if(!zoom && width > maxwidth) {
        image_set_scale(image, (double)maxwidth / width);

        draw_image(image, x, y, maxwidth, height * maxwidth / width, 0, 0);

        image_set_scale(image, 1.0);
    } else {
        if(width > maxwidth) {
            draw_image(image, x, y, maxwidth, height, (int)((double)(width - maxwidth) * position), 0);
        } else {
            draw_image(image, x, y, width, height, 0, 0);
        }
    }
}

/* Called by new file transfer to add a new message to the msg list */
MSG_FILE* message_add_type_file(FILE_TRANSFER *file){//TODO shove on ui thread
    MSG_FILE *msg = malloc(sizeof(MSG_FILE));
    msg->author = file->incoming ? 0 : 1;
    msg->msg_type = MSG_TYPE_FILE;
    msg->filenumber = file->file_number;
    msg->status = file->status;
        // msg->name_length is the max enforce that
    msg->name_length = (file->name_length > sizeof(msg->name)) ? sizeof(msg->name) : file->name_length;
    memcpy(msg->name, file->name, msg->name_length);
    msg->size = file->size;
    msg->progress = file->size_transferred;
    msg->speed = 0;
    msg->inline_png = file->in_memory;
    msg->path = NULL;

    // FRIEND *f = &friend[file->friend_number];
    // *str = file_translate_status(*file->status);

    return msg;
}

/** Formats all messages from self and friends, and then call draw functions
 * to write them to the UI.
 *
 * accepts: messages struct *pointer, int x,y positions, int width,height
 */
void messages_draw(MESSAGES *m, int x, int y, int width, int height) {
    // Do not draw author name next to every message
    uint8_t lastauthor = 0xFF;

    // Message iterator
    void **p = m->data->data;
    MSG_IDX i, n = m->data->n;
    y += 0;//2 * SCALE;

    // Go through messages
    for(i = 0; i != n; i++) {
        MESSAGE *msg = *p++;

        // Empty message
        if(msg->height == 0) {
            return;
        }

        //! NOTE: should not be constant 0
        if (y + msg->height <= 0) {
            y += msg->height;
            continue;
        }

        //! NOTE: should not be constant 100
        if(y >= height + 50 * SCALE) {
            break;
        }

        // Draw timestamps
        {
            char timestr[6];
            STRING_IDX len;
            len = snprintf(timestr, sizeof(timestr), "%u:%.2u", msg->time / 60, msg->time % 60);
            if (len >= sizeof(timestr)) {
                len = sizeof(timestr) - 1;
            }

            setcolor(COLOR_MAIN_SUBTEXT);
            setfont(FONT_MISC);
            drawtext(x + width - ACTUAL_TIME_WIDTH, y, (char_t*)timestr, len);
        }

        // Draw the names for groups or friends
        if (m->type) {
            // Group message authors are all the same color
            setcolor(COLOR_MAIN_CHATTEXT);
            setfont(FONT_TEXT);
            drawtextwidth_right(x, MESSAGES_X - NAME_OFFSET, y, &msg->msg[msg->length] + 1, msg->msg[msg->length]);
        } else {
            FRIEND *f = &friend[m->data->id];

            // Always draw name next to action message
            if (msg->msg_type == MSG_TYPE_ACTION_TEXT) {
                lastauthor = 0xFF;
            }

            if (msg->author != lastauthor) {
                // Draw author name
                // If author is current user
                setfont(FONT_TEXT);
                if (msg->msg_type == MSG_TYPE_ACTION_TEXT) {
                    setcolor(COLOR_MAIN_ACTIONTEXT);
                } else {
                    if (msg->author) {
                        setcolor(COLOR_MAIN_SUBTEXT);
                    } else {
                        setcolor(COLOR_MAIN_CHATTEXT);
                    }
                }

                if (msg->author) {
                    drawtextwidth_right(x, MESSAGES_X - NAME_OFFSET, y, self.name, self.name_length);
                } else if (f->alias) {
                    drawtextwidth_right(x, MESSAGES_X - NAME_OFFSET, y, f->alias, f->alias_length);
                } else {
                    drawtextwidth_right(x, MESSAGES_X - NAME_OFFSET, y, f->name, f->name_length);
                }

                lastauthor = msg->author;
            }
        }

        // Draw message contents
        switch(msg->msg_type) {
        case MSG_TYPE_TEXT:
        case MSG_TYPE_ACTION_TEXT: {
            // Normal message
            STRING_IDX h1 = STRING_IDX_MAX, h2 = STRING_IDX_MAX;
            if(i == m->data->istart) {
                h1 = m->data->start;
                h2 = ((i == m->data->iend) ? m->data->end : msg->length);
            } else if(i == m->data->iend) {
                h1 = 0;
                h2 = m->data->end;
            } else if(i > m->data->istart && i < m->data->iend) {
                h1 = 0;
                h2 = msg->length;
            }

            if((m->data->istart == m->data->iend && m->data->start == m->data->end) || h1 == h2) {
                h1 = STRING_IDX_MAX;
                h2 = STRING_IDX_MAX;
            }

            if(msg->author) {
                setcolor(COLOR_MAIN_SUBTEXT);
            } else {
                setcolor(COLOR_MAIN_CHATTEXT);
            }

            if (msg->msg_type == MSG_TYPE_ACTION_TEXT) {
                setcolor(COLOR_MAIN_ACTIONTEXT);
            }

            setfont(FONT_TEXT);
            int ny = drawtextmultiline(x + MESSAGES_X, x + width - TIME_WIDTH,
                                                    y,               MAIN_TOP,
                                       y + msg->height, font_small_lineheight,
                                       msg->msg, msg->length, h1, h2 - h1, 0, 0, 1);

            if(ny < y || (uint32_t)(ny - y) + MESSAGES_SPACING != msg->height) {
                debug("error101 %u %u\n", ny -y, msg->height - MESSAGES_SPACING);
            }
            y = ny;

            break;
        }

        // Draw image
        case MSG_TYPE_IMAGE: {
            MSG_IMG *img = (void*)msg;
            int maxwidth = width - MESSAGES_X - TIME_WIDTH;
            draw_message_image(img->image, x + MESSAGES_X, y, img->w, img->h, maxwidth, img->zoom, img->position);
            y += (img->zoom || img->w <= maxwidth) ? img->h : img->h * maxwidth / img->w;
            break;
        }

        // Draw file transfer
        case MSG_TYPE_FILE: {
            MSG_FILE *file = (void*)msg;
            int dx      = x + MESSAGES_X;
            int d_width = width - MESSAGES_X - TIME_WIDTH;
            /* Mouse Positions */
            _Bool mo = (m->iover == i);
            _Bool mouse_over = (mo && m->over) ? 1 : 0;
            _Bool mouse_tbtn = (mo && m->over == 1) ? 1 : 0;
            _Bool mouse_bbtn = (mo && m->over == 2) ? 1 : 0;

            /* Old var kept for bug hunting
            int xx = x + dx;
            int xxx = xx + BM_FTM_WIDTH + SCALE; */

            /* Button Background */
            int btn_bg_w  = BM_FTB_WIDTH;
            /* Button Background heights */
            int tbtn_bg_y = y;
            int bbtn_bg_y = y + BM_FTB_HEIGHT + SCALE * 2;
            int tbtn_bg_h = BM_FTB_HEIGHT + SCALE;
            int bbtn_bg_h = BM_FTB_HEIGHT;
            /* Top button info */
            int btnx   = x + width - TIME_WIDTH;
            int tbtn_y = y + SCALE * 4;
            int bbtn_y = y + BM_FTB_HEIGHT + SCALE * 5;
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

            uint8_t    text_name_and_size[file->name_length + 33];
            memcpy(text_name_and_size, file->name, file->name_length);
            text_name_and_size[file->name_length] = ' ';
            STRING_IDX text_name_and_size_len = file->name_length + 1;
            text_name_and_size_len += sprint_humanread_bytes(text_name_and_size + file->name_length + 1, 32, file_size);

            uint8_t    text_speed[32];
            STRING_IDX text_speed_len = sprint_humanread_bytes(text_speed, sizeof(text_speed), file_speed);
            if (text_speed_len <= 30) {
                text_speed[text_speed_len++] = '/';
                text_speed[text_speed_len++] = 's';
            }

            uint8_t    text_ttc[32];
            STRING_IDX text_ttc_len = snprintf((char*)text_ttc, sizeof(text_ttc), "%llus", file_ttc);
            if (text_ttc_len >= sizeof(text_ttc)) {
                text_ttc_len = sizeof(text_ttc) - 1;
            }

            // progress rectangle
            uint32_t prog_box = d_width; // - 10 * SCALE;
            uint32_t prog_bar = (file->size == 0) ? 0 : ((long double)prog_box * file_percent);

            setfont(FONT_MISC);
            setcolor(COLOR_BACKGROUND_MAIN);

            /* Draw macros added, to reduce future line edits. */
            #define draw_ft_rect(color) draw_rect_fill (dx, y, d_width, FILE_TRANSFER_BOX_HEIGHT, color)
            #define draw_ft_prog(color) draw_rect_fill (dx, y, prog_bar, FILE_TRANSFER_BOX_HEIGHT, color)

            // #define draw_ft_prog(color) draw_rect_frame(dx + 5 * SCALE, y + 17 * SCALE, prog_box, 7 * SCALE, color);\
            //                            draw_rect_fill (dx + 5 * SCALE, y + 17 * SCALE, prog_bar, 7 * SCALE, color)

            int wbound = dx + d_width - 3 * SCALE;

            #define draw_ft_text_right(str, len) do { wbound -= (textwidth(str, len) + (6 * SCALE)); drawtext(wbound, y + 3 * SCALE, str, len); } while (0)
            #define drawstr_ft_right(t) draw_ft_text_right(S(t), SLEN(t))

            switch (file->status){
            case FILE_TRANSFER_STATUS_COMPLETED: {
                /* If mouse over use hover color */
                setcolor((mouse_over) ? COLOR_BUTTON_SUCCESS_HOVER_TEXT : COLOR_BUTTON_SUCCESS_TEXT);
                draw_ft_rect(mouse_over ? COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND : COLOR_BUTTON_SUCCESS_BACKGROUND);

                drawalpha(BM_YES, btnx, tbtn_y, btnw, btnh,
                          (mouse_over) ? COLOR_BUTTON_SUCCESS_HOVER_TEXT : COLOR_BUTTON_SUCCESS_TEXT);
                if (file->inline_png) {
                    drawstr_ft_right(CLICKTOSAVE);
                } else {
                    drawstr_ft_right(CLICKTOOPEN);
                }
                break;
            }
            case FILE_TRANSFER_STATUS_KILLED:{
                setcolor(COLOR_BUTTON_DANGER_TEXT);
                draw_ft_rect(COLOR_BUTTON_DANGER_BACKGROUND);
                drawalpha(BM_NO, btnx, tbtn_y, btnw, btnh, COLOR_BUTTON_DANGER_TEXT);
                drawstr_ft_right(TRANSFER_CANCELLED);
                break;
            }
            case FILE_TRANSFER_STATUS_BROKEN: {
                setcolor(COLOR_BUTTON_DANGER_TEXT);
                draw_ft_rect(COLOR_BUTTON_DANGER_BACKGROUND);
                drawalpha(BM_NO, btnx, tbtn_y, btnw, btnh, COLOR_BUTTON_DANGER_TEXT);
                drawstr_ft_right(TRANSFER_BROKEN);
                break;
            }
            case FILE_TRANSFER_STATUS_NONE: {
                /* ↑ used for incoming transfers */
                setcolor(COLOR_BUTTON_DISABLED_TRANSFER);
                draw_ft_rect(COLOR_BUTTON_DISABLED_BACKGROUND);

                drawalpha(BM_FTB1, btnx, tbtn_bg_y, btn_bg_w, tbtn_bg_h,
                          (mouse_tbtn ? COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND : COLOR_BUTTON_SUCCESS_BACKGROUND));
                drawalpha(BM_NO, btnx, tbtn_y, btnw, btnh,
                          (mouse_tbtn ? COLOR_BUTTON_SUCCESS_HOVER_TEXT : COLOR_BUTTON_SUCCESS_TEXT));

                drawalpha(BM_FTB2, btnx, bbtn_bg_y, btn_bg_w, bbtn_bg_h,
                          (mouse_bbtn ? COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND : COLOR_BUTTON_SUCCESS_BACKGROUND));
                drawalpha(BM_YES, btnx, bbtn_y, btnw, btnh,
                          (mouse_bbtn ? COLOR_BUTTON_SUCCESS_HOVER_TEXT : COLOR_BUTTON_SUCCESS_TEXT));

                draw_ft_prog(COLOR_BUTTON_DISABLED_FOREGROUND);
                break;
            }
            case FILE_TRANSFER_STATUS_ACTIVE: {
                setcolor(COLOR_BUTTON_INPROGRESS_TEXT);
                draw_ft_rect(COLOR_BUTTON_INPROGRESS_BACKGROUND);

                drawalpha(BM_FTB1, btnx, tbtn_bg_y, btn_bg_w, tbtn_bg_h,
                          (mouse_tbtn ? COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND : COLOR_BUTTON_SUCCESS_BACKGROUND));
                drawalpha(BM_NO, btnx, tbtn_y, btnw, btnh,
                          (mouse_tbtn ? COLOR_BUTTON_DANGER_HOVER_TEXT : COLOR_BUTTON_DANGER_TEXT));

                drawalpha(BM_FTB2, btnx, bbtn_bg_y, btn_bg_w, bbtn_bg_h,
                          (mouse_bbtn ? COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND : COLOR_BUTTON_SUCCESS_BACKGROUND));
                drawalpha(BM_PAUSE, btnx, bbtn_y, btnw, btnh,
                          (mouse_bbtn ? COLOR_BUTTON_SUCCESS_HOVER_TEXT : COLOR_BUTTON_SUCCESS_TEXT));

                draw_ft_prog(COLOR_BUTTON_INPROGRESS_FOREGROUND);
                draw_ft_text_right(text_ttc, text_ttc_len);
                draw_ft_text_right(text_speed, text_speed_len);
                break;
            }
            case FILE_TRANSFER_STATUS_PAUSED_US:
            case FILE_TRANSFER_STATUS_PAUSED_BOTH:
            case FILE_TRANSFER_STATUS_PAUSED_THEM: {
                setcolor(COLOR_BUTTON_DISABLED_TRANSFER);

                draw_ft_rect(COLOR_BUTTON_DISABLED_BACKGROUND);

                drawalpha(BM_FTB1, btnx, tbtn_bg_y, btn_bg_w, tbtn_bg_h,
                          (mouse_tbtn ? COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND : COLOR_BUTTON_SUCCESS_BACKGROUND));
                drawalpha(BM_NO, btnx, tbtn_y, btnw, btnh,
                          (mouse_tbtn ? COLOR_BUTTON_DANGER_HOVER_TEXT : COLOR_BUTTON_DANGER_TEXT));

                if(file->status <= FILE_TRANSFER_STATUS_PAUSED_BOTH){
                    /* Paused by at least us */
                    drawalpha(BM_FTB2, btnx, bbtn_bg_y, btn_bg_w, bbtn_bg_h,
                              (mouse_bbtn ? COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND : COLOR_BUTTON_SUCCESS_BACKGROUND));
                    drawalpha(BM_RESUME, btnx, bbtn_y, btnw, btnh,
                              (mouse_bbtn ? COLOR_BUTTON_SUCCESS_HOVER_TEXT : COLOR_BUTTON_SUCCESS_TEXT));
                } else {
                    /* Paused only by them */
                    drawalpha(BM_FTB2, btnx, bbtn_bg_y, btn_bg_w, bbtn_bg_h, COLOR_BUTTON_DISABLED_BACKGROUND);
                    drawalpha(BM_PAUSE, btnx, bbtn_y, btnw, btnh, COLOR_BUTTON_DISABLED_TRANSFER);
                }

                draw_ft_prog(COLOR_BUTTON_DISABLED_FOREGROUND);
                break;
            }
            }

            drawalpha(BM_FILE, x + 7 * SCALE, y + 2 * SCALE,
                      BM_FILE_WIDTH, BM_FILE_HEIGHT,
                      COLOR_BUTTON_DISABLED_TRANSFER);

            // textwidth()
            // drawtextwidth_right(dx + 5 * SCALE, y + 10 * SCALE, text_size, text_size_len);
            drawtextrange(dx + 5 * SCALE, wbound - 5 * SCALE, y + 3 * SCALE, text_name_and_size, text_name_and_size_len);
            //drawtext(os + 3 * SCALE, y + 3 * SCALE, text_size, text_size_len);

            y += BM_FT_HEIGHT;
            break;
        }
        }

        y += MESSAGES_SPACING;
    }
}

_Bool messages_mmove(MESSAGES *m, int UNUSED(px), int UNUSED(py), int width, int UNUSED(height),
                     int mx, int my, int dx, int UNUSED(dy)) {
    if(m->idown < m->data->n) {
        int maxwidth = width - MESSAGES_X - TIME_WIDTH;
        MSG_IMG *img_down = m->data->data[m->idown];
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

    if(mx < 0 || my < 0 || (uint32_t) my > m->data->height) {
        if(m->iover != MSG_IDX_MAX) {
            m->iover = MSG_IDX_MAX;
            return 1;
        }
        return 0;
    }

    setfont(FONT_TEXT);

    void **p = m->data->data;
    MSG_IDX i = 0, n = m->data->n;
    _Bool need_redraw = 0;

    while(i != n) {
        MESSAGE *msg = *p++;

        int dy = msg->height;

        if(my >= 0 && my < dy) {
            switch(msg->msg_type) {
            case MSG_TYPE_TEXT:
            case MSG_TYPE_ACTION_TEXT: {
                /* normal message */
                m->over = hittextmultiline(mx - MESSAGES_X, width - MESSAGES_X - TIME_WIDTH, my < 0 ? 0 : my,
                                           msg->height, font_small_lineheight, msg->msg, msg->length, 1);

                _Bool prev_urlmdown = m->urlmdown;
                if (m->urlover != STRING_IDX_MAX) {
                    m->urlmdown = 0;
                    m->urlover = STRING_IDX_MAX;
                }

                if(my < 0 || my >= dy || mx < MESSAGES_X || m->over == msg->length) {
                    break;
                }

                cursor = CURSOR_TEXT;

                char_t *str = msg->msg + m->over;
                while(str != msg->msg) {
                    str--;
                    if(*str == ' ' || *str == '\n') {
                        str++;
                        break;
                    }
                }

                char_t *end = msg->msg + msg->length;
                while(str != end && *str != ' ' && *str != '\n') {
                    if(( str == msg->msg || *(str - 1) == '\n' || *(str - 1) == ' ') &&
                       (m->urlover == STRING_IDX_MAX && end - str >= 7 && strcmp2(str, "http://") == 0)) {
                        cursor = CURSOR_HAND;
                        m->urlover = str - msg->msg;
                    }

                    if(( str == msg->msg || *(str - 1) == '\n' || *(str - 1) == ' ') &&
                       (m->urlover == STRING_IDX_MAX && end - str >= 8 && strcmp2(str, "https://") == 0)) {
                        cursor = CURSOR_HAND;
                        m->urlover = str - msg->msg;
                    }

                    str++;
                }

                if(m->urlover != STRING_IDX_MAX) {
                    m->urllen = (str - msg->msg) - m->urlover;
                    m->urlmdown = prev_urlmdown;
                }

                break;
            }

            case MSG_TYPE_IMAGE: {
                m->over = 0;

                MSG_IMG *img = (void*)msg;
                int maxwidth = width - MESSAGES_X - TIME_WIDTH;

                if(img->w > maxwidth) {
                    mx -= MESSAGES_X;
                    int w = img->w > maxwidth ? maxwidth : img->w;
                    int h = (img->zoom || img->w <= maxwidth) ? img->h : img->h * maxwidth / img->w;
                    if(mx >= 0 && my >= 0 && mx < w && my < h) {
                        m->over = 1;
                        cursor = CURSOR_ZOOM_IN + img->zoom;
                    }
                }
                break;
            }

            case MSG_TYPE_FILE: {
                uint8_t over = 0;
                MSG_FILE *file = (void*)msg;

                mx -= 5 * SCALE;
                if (mx >= 0 && mx < width &&
                    my >= 0 && my < FILE_TRANSFER_BOX_HEIGHT) {
                    over = 3;
                    if(mx >= width - TIME_WIDTH) {
                        if(my < BM_FTB_HEIGHT + SCALE) {
                            // mouse is over the upper button (cancel)
                            over = 1;
                        } else if(my >= BM_FTB_HEIGHT + SCALE * 2) {
                            // mouse is over the lower button (pause / accept)
                            over = 2;
                        }
                    }
                }

                /* WTF IS THIS?! REALLY?! I MEAN SRSLY... WTF! */
                /* It was hard to write, it should be hard to understand IS A JOKE! */
                static const uint8_t f[16] = {
                    0b011,
                    0b001,

                    0b011,
                    0b011,

                    0b011,
                    0b011,

                    0b001,
                    0b001,

                    0b001,
                    0b001,

                    0b001,
                    0b001,

                    0b000,
                    0b000,

                    0b111,
                    0b111,
                };

                if(over && f[file->status * 2 + file->author] & (1 << (over - 1))) {
                    cursor = CURSOR_HAND;
                }

                if(over != m->over) {
                    need_redraw = 1;
                    m->over = over;
                }

                break;
            }
            }

            if((i != m->iover) && (m->iover != MSG_IDX_MAX) && ((msg->msg_type == MSG_TYPE_FILE) ||
               (((MESSAGE*)(m->data->data[m->iover]))->msg_type == MSG_TYPE_FILE))) {
                need_redraw = 1; // Redraw file on hover-in/out.
            }

            m->iover = i;

            if(m->select) {
                MSG_IDX istart, iend;
                STRING_IDX start, end;
                if(i > m->idown) {
                    istart = m->idown;
                    iend = i;

                    start = m->down;
                    end = m->over;
                } else if(i < m->idown) {
                    iend = m->idown;
                    istart = i;

                    end = m->down;
                    start = m->over;
                } else {
                    istart = iend = i;
                    if(m->over >= m->down) {
                        start = m->down;
                        end = m->over;
                    } else {
                        end = m->down;
                        start = m->over;
                    }
                }

                if (start != m->data->start ||
                    istart != m->data->istart ||
                    end != m->data->end ||
                    iend != m->data->iend) {

                    m->data->start = start;
                    m->data->end = end;
                    m->data->istart = istart;
                    m->data->iend = iend;
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

_Bool messages_mdown(MESSAGES *m) {
    m->idown = MSG_IDX_MAX;
    if(m->iover != MSG_IDX_MAX) {
        MESSAGE *msg = m->data->data[m->iover];
        switch(msg->msg_type) {
        case MSG_TYPE_TEXT:
        case MSG_TYPE_ACTION_TEXT: {
            if(m->urlover != STRING_IDX_MAX) {
                m->urlmdown = 1;
            }

            m->data->istart = m->data->iend = m->idown = m->iover;
            m->data->start = m->data->end = m->down = m->over;
            m->select = 1;
            break;
        }

        case MSG_TYPE_IMAGE: {
            MSG_IMG *img = (void*)msg;
            if(m->over) {
                if(!img->zoom) {
                    img->zoom = 1;
                    message_updateheight(m, msg, m->data);
                } else {
                    m->idown = m->iover;
                }
            }
            break;
        }

        case MSG_TYPE_FILE: {
            MSG_FILE *file = (void*)msg;
            if(m->over == 0) {
                break;
            }

            switch(file->status) {
            case FILE_TRANSFER_STATUS_NONE: {
                if(!msg->author) {
                    if(m->over == 2) {
                        savefilerecv(m->data->id, file);
                    } else if(m->over == 1) {
                        //decline
                        tox_postmessage(TOX_FILE_CANCEL, m->data->id, file->filenumber, NULL);
                    }
                } else if(m->over == 1) {
                    //cancel
                    tox_postmessage(TOX_FILE_CANCEL, m->data->id, file->filenumber, NULL);
                }


                break;
            }

            case FILE_TRANSFER_STATUS_ACTIVE: {
                if(m->over == 2) {
                    //pause
                    tox_postmessage(TOX_FILE_PAUSE, m->data->id, file->filenumber, NULL);
                } else if(m->over == 1) {
                    //cancel
                    tox_postmessage(TOX_FILE_CANCEL, m->data->id, file->filenumber, NULL);
                }
                break;
            }

            case FILE_TRANSFER_STATUS_PAUSED_US: {
                if(m->over == 2) {
                    //resume
                    tox_postmessage(TOX_FILE_RESUME, m->data->id, file->filenumber, NULL);
                } else if(m->over == 1) {
                    //cancel
                    tox_postmessage(TOX_FILE_CANCEL, m->data->id, file->filenumber, NULL);
                }
                break;
            }

            case FILE_TRANSFER_STATUS_PAUSED_THEM:
            case FILE_TRANSFER_STATUS_BROKEN: {
                //cancel
                if(m->over == 1) {
                    tox_postmessage(TOX_FILE_CANCEL, m->data->id, file->filenumber, NULL);
                }
                break;
            }

            case FILE_TRANSFER_STATUS_COMPLETED: {
                if(m->over) {
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
        if(m->data->istart != m->data->iend || m->data->start != m->data->end) {
            m->data->istart = 0;
            m->data->iend = 0;
            m->data->start = 0;
            m->data->end = 0;
            return 1;
        }
    }

    return 0;
}

_Bool messages_dclick(MESSAGES *m, _Bool triclick)
{
    if(m->iover != MSG_IDX_MAX) {
        MESSAGE *msg = m->data->data[m->iover];
        switch(msg->msg_type) {
        case MSG_TYPE_TEXT:
        case MSG_TYPE_ACTION_TEXT: {
            m->data->istart = m->data->iend = m->iover;

            char_t c = triclick ? '\n' : ' ';

            STRING_IDX i = m->over;
            while(i != 0 && msg->msg[i - 1] != c) {
                i -= utf8_unlen(msg->msg + i);
            }
            m->data->start = i;
            i = m->over;
            while(i != msg->length && msg->msg[i] != c) {
                i += utf8_len(msg->msg + i);
            }
            m->data->end = i;
            return 1;
        }
        case MSG_TYPE_IMAGE: {
            MSG_IMG *img = (void*)msg;
            if(m->over) {
                if(img->zoom) {
                    img->zoom = 0;
                    message_updateheight(m, msg, m->data);
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

_Bool messages_mright(MESSAGES *m)
{
    static UI_STRING_ID menu_copy[] = {STR_COPY, STR_COPY_WITH_NAMES};
    if(m->iover == MSG_IDX_MAX) {
        return 0;
    }

    MESSAGE* msg = (MESSAGE*)m->data->data[m->iover];

    switch(msg->msg_type) {
    case MSG_TYPE_TEXT:
    case MSG_TYPE_ACTION_TEXT: {
        contextmenu_new(countof(menu_copy), menu_copy, contextmenu_messages_onselect);
        return 1;
    }
    }
    return 0;
}

_Bool messages_mwheel(MESSAGES *UNUSED(m), int UNUSED(height), double UNUSED(d))
{
    return 0;
}


_Bool messages_mup(MESSAGES *m){

    if(m->iover != MSG_IDX_MAX) {
        MESSAGE *msg = m->data->data[m->iover];
        if(msg->msg_type == MSG_TYPE_TEXT){
            if(m->urlover != STRING_IDX_MAX && m->urlmdown) {
                char_t url[m->urllen + 1];
                memcpy(url, msg->msg + m->urlover, m->urllen * sizeof(char_t));
                url[m->urllen] = 0;
                openurl(url);
                m->urlmdown = 0;
            }
        }
    }

    //temporary, change this
    if(m->select) {
        char_t *lel = malloc(65536); //TODO: De-hardcode this value.
        setselection(lel, messages_selection(m, lel, 65536, 0));
        free(lel);


        m->select = 0;
    }

    m->idown = MSG_IDX_MAX;

    return 0;
}

_Bool messages_mleave(MESSAGES *UNUSED(m))
{
    return 0;
}

int messages_selection(MESSAGES *m, void *buffer, uint32_t len, _Bool names)
{
    if(m->data->n == 0) {
        *(char_t*)buffer = 0;
        return 0;
    }

    MSG_IDX i = m->data->istart, n = m->data->iend + 1;
    void **dp = &m->data->data[i];

    char_t *p = buffer;

    while(i != MSG_IDX_MAX && i != n) {
        MESSAGE *msg = *dp++;

        if(names && (i != m->data->istart || m->data->start == 0)) {
            if(m->type) {
                //TODO: get rid of such hacks or provide unpacker.
                //This basically undoes copy_groupmessage().
                uint8_t l = (uint8_t)msg->msg[msg->length];
                if(len <= l) {
                    break;
                }

                memcpy(p, &msg->msg[msg->length + 1], l);
                p += l;
                len -= l;
            } else {
                FRIEND *f = &friend[m->data->id];

                if(!msg->author) {
                    if(len <= f->name_length) {
                        break;
                    }

                    memcpy(p, f->name, f->name_length);
                    p += f->name_length;
                    len -= f->name_length;
                } else {
                    if(len <= self.name_length) {
                        break;
                    }

                    memcpy(p, self.name, self.name_length);
                    p += self.name_length;
                    len -= self.name_length;
                }
            }

            if(len <= 2) {
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
            STRING_IDX length;
            if(i == m->data->istart) {
                if(i == m->data->iend) {
                    data = msg->msg + m->data->start;
                    length = m->data->end - m->data->start;
                } else {
                    data = msg->msg + m->data->start;
                    length = msg->length - m->data->start;
                }
            } else if(i == m->data->iend) {
                data = msg->msg;
                length = m->data->end;
            } else {
                data = msg->msg;
                length = msg->length;
            }

            if(len <= length) {
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

static int msgheight(MESSAGE *msg, int width)
{
    switch(msg->msg_type) {
    case MSG_TYPE_TEXT:
    case MSG_TYPE_ACTION_TEXT: {
        int theight = text_height(abs(width - MESSAGES_X - TIME_WIDTH), font_small_lineheight, msg->msg, msg->length);
        return (theight == 0) ? 0 : theight + MESSAGES_SPACING;
    }

    case MSG_TYPE_IMAGE: {
        MSG_IMG *img = (void*)msg;
        int maxwidth = width - MESSAGES_X - TIME_WIDTH;
        return ((img->zoom || img->w <= maxwidth) ? img->h : img->h * maxwidth / img->w) + MESSAGES_SPACING;
    }

    case MSG_TYPE_FILE: {
        return BM_FT_HEIGHT + MESSAGES_SPACING;
    }

    }

    return 0;
}

void messages_updateheight(MESSAGES *m)
{
    MSG_DATA *data = m->data;
    if(!data) {
        return;
    }

    setfont(FONT_TEXT);

    uint32_t height = 0;
    MSG_IDX i = 0;
    while(i < data->n) {
        MESSAGE *msg = data->data[i];
        msg->height = msgheight(msg, m->width);
        height += msg->height;
        i++;
    }

    m->height = height;
    data->height = height;
    data->width = m->width;
    m->panel.content_scroll->content_height = height;
}

static void message_setheight(MESSAGES *m, MESSAGE *msg, MSG_DATA *p)
{
    if(m->width == 0) {
        return;
    }

    setfont(FONT_TEXT);

    msg->height = msgheight(msg, m->width);
    p->height += msg->height;
    if(m->data == p) {
        m->panel.content_scroll->content_height = p->height;
    }
}

void message_updateheight(MESSAGES *m, MESSAGE *msg, MSG_DATA *p)
{
    if(m->width == 0) {
        return;
    }

    setfont(FONT_TEXT);

    p->height -= msg->height;
    msg->height = msgheight(msg, m->width);
    p->height += msg->height;
    if(m->data == p) {
        m->panel.content_scroll->content_height = p->height;
    }
}

/** Appends a messages from self or friend to the message list;
 * will realloc or trim messages as needed;
 *
 * also handles auto scrolling selections with messages
 *
 * accepts: MESSAGES *pointer, MESSAGE *pointer, MSG_DATA *pointer
 */
void message_add(MESSAGES *m, MESSAGE *msg, MSG_DATA *p)
{
    time_t rawtime;
    struct tm *ti;
    time(&rawtime);
    ti = localtime(&rawtime);

    // Set the time this message was received by utox
    msg->time = ti->tm_hour * 60 + ti->tm_min;

    if(p->n < MAX_BACKLOG_MESSAGES) {
        p->data = realloc(p->data, (p->n + 1) * sizeof(void*));
        p->data[p->n++] = msg;
    } else {
        p->height -= ((MESSAGE*)p->data[0])->height;
        message_free(p->data[0]);
        memmove(p->data, p->data + 1, (MAX_BACKLOG_MESSAGES - 1) * sizeof(void*));
        p->data[MAX_BACKLOG_MESSAGES - 1] = msg;

        // Scroll selection up so that it stays over the same messages.
        if (p->istart != MSG_IDX_MAX) {
            if(0 < p->istart) {
                p->istart--;
            } else {
                p->start = 0;
            }
        }
        if (p->iend != MSG_IDX_MAX) {
            if(0 < p->iend) {
                p->iend--;
            } else {
                p->end = 0;
            }
        }
        if (p == m->data) {
            if (m->idown != MSG_IDX_MAX) {
                if(0 < m->idown) {
                    m->idown--;
                } else {
                    m->down = 0;
                }
            }
            if (m->iover != MSG_IDX_MAX) {
                if(0 < m->iover) {
                    m->iover--;
                } else {
                    m->over = 0;
                }
            }
        }
    }

    message_setheight(m, msg, p);
}

_Bool messages_char(uint32_t ch)
{
    MESSAGES *m;
    if(selected_item->item == ITEM_FRIEND) {
        m = &messages_friend;
    } else if(selected_item->item == ITEM_GROUP) {
        m = &messages_group;
    } else {
        return 0;
    }

    switch(ch) {
        //!TODO: not constant 0.25
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

void message_free(MESSAGE *msg)
{
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

void message_clear(MESSAGES *m, MSG_DATA *p)
{
    MSG_IDX i;

    for(i = 0; i < p->n; i++)
    {
        message_free((MESSAGE*)p->data[i]);
    }

    free(p->data);
    p->data = NULL;
    p->n = 0;

    p->istart = p->iend = p->start = p->end = 0;

    p->height = 0;
    if(m->data == p) {
        m->panel.content_scroll->content_height = p->height;
    }
}
