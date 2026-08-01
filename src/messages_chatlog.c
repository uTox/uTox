#include "messages.h"

#include "chatlog.h"
#include "debug.h"

#include "ui/scrollable.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

bool messages_day_changed(time_t last, time_t next) {
    if (last == 0) {
        return false;
    }

    int ltime_year = 0, ltime_mon = 0, ltime_day = 0;
    struct tm *msg_time = localtime(&last);

    ltime_year = msg_time->tm_year;
    ltime_mon  = msg_time->tm_mon;
    ltime_day  = msg_time->tm_mday;
    msg_time   = localtime(&next);

    if (ltime_year >= msg_time->tm_year
        && (ltime_year != msg_time->tm_year || ltime_mon >= msg_time->tm_mon)
        && (ltime_year != msg_time->tm_year || ltime_mon != msg_time->tm_mon || ltime_day >= msg_time->tm_mday))
    {
        return false;
    }

    return true;
}

MSG_HEADER *messages_create_day_notice(time_t next) {
    struct tm *msg_time = localtime(&next);

    MSG_HEADER *msg = calloc(1, sizeof(MSG_HEADER));
    if (!msg) {
        LOG_ERR("Messages", "Couldn't allocate memory for day notice.");
        return NULL;
    }

    time(&msg->time);
    msg->our_msg  = 0;
    msg->msg_type = MSG_TYPE_NOTICE_DAY_CHANGE;

    msg->via.notice_day.msg = calloc(1, 256);
    if (!msg->via.notice_day.msg) {
        LOG_ERR("Messages", "Couldn't allocate memory for day notice text.");
        free(msg);
        return NULL;
    }

    msg->via.notice_day.length =
        strftime((char *)msg->via.notice_day.msg, 256, "Day has changed to %A %B %d %Y", msg_time);
    if (0 == msg->via.notice_day.length) {
        LOG_ERR("Messages", "Couldn't compose day notice message.");
        free(msg->via.notice_day.msg);
        free(msg);
        return NULL;
    }

    return msg;
}

static void messages_bump_indices_for_prepend(MESSAGES *m, size_t n) {
    if (m->sel_start_msg != UINT32_MAX) {
        m->sel_start_msg += n;
    }
    if (m->sel_end_msg != UINT32_MAX) {
        m->sel_end_msg += n;
    }
    if (m->cursor_down_msg != UINT32_MAX) {
        m->cursor_down_msg += n;
    }
    if (m->cursor_over_msg != UINT32_MAX) {
        m->cursor_over_msg += n;
    }
}

static void messages_drop_newest(MESSAGES *m) {
    if (m->number == 0) {
        return;
    }

    MSG_HEADER *msg = m->data[m->number - 1];
    if (m->width != 0) {
        m->height -= msg->height;
    }
    message_free(msg);
    m->number--;
    m->extra++;

    if (m->sel_start_msg != UINT32_MAX && m->sel_start_msg >= m->number) {
        m->sel_start_msg      = m->number ? m->number - 1 : UINT32_MAX;
        m->sel_start_position = 0;
    }
    if (m->sel_end_msg != UINT32_MAX && m->sel_end_msg >= m->number) {
        m->sel_end_msg      = m->number ? m->number - 1 : UINT32_MAX;
        m->sel_end_position = 0;
    }
    if (m->cursor_down_msg != UINT32_MAX && m->cursor_down_msg >= m->number) {
        m->cursor_down_msg      = m->number ? m->number - 1 : UINT32_MAX;
        m->cursor_down_position = 0;
    }
    if (m->cursor_over_msg != UINT32_MAX && m->cursor_over_msg >= m->number) {
        m->cursor_over_msg      = m->number ? m->number - 1 : UINT32_MAX;
        m->cursor_over_position = 0;
    }
}

static void messages_adjust_scroll_after_prepend(MESSAGES *m, int old_height) {
    SCROLLABLE *scroll = m->panel.content_scroll;
    if (!scroll || m->height <= 0) {
        return;
    }

    const int viewport = scroll->viewport_height;
    scroll->content_height = m->height;

    if (viewport <= 0 || m->height <= viewport) {
        if (m->height > old_height) {
            scroll->d = (double)(m->height - old_height) / (double)m->height;
            if (scroll->d > 1.0) {
                scroll->d = 1.0;
            }
        }
        return;
    }

    int scroll_y = 0;
    if (old_height > viewport) {
        scroll_y = (int)(scroll->d * (double)(old_height - viewport) + 0.5);
    }
    scroll_y += m->height - old_height;

    if (scroll_y <= UTOX_CHATLOG_LOAD_NEAR_TOP) {
        scroll_y = m->height - old_height;
    }

    scroll->d = (double)scroll_y / (double)(m->height - viewport);
    if (scroll->d < 0.0) {
        scroll->d = 0.0;
    } else if (scroll->d > 1.0) {
        scroll->d = 1.0;
    }
}

size_t messages_prepend(MESSAGES *m, MSG_HEADER **batch, size_t n) {
    if (!m || !batch || n == 0) {
        return 0;
    }

    pthread_mutex_lock(&messages_lock);

    MSG_HEADER **expanded = calloc(n * 2 + 1, sizeof(MSG_HEADER *));
    if (!expanded) {
        LOG_ERR("Messages", "Failed to allocate expanded prepend buffer.");
        pthread_mutex_unlock(&messages_lock);
        return 0;
    }

    size_t out  = 0;
    time_t last = 0;
    for (size_t i = 0; i < n; i++) {
        MSG_HEADER *msg = batch[i];
        if (!msg) {
            continue;
        }
        if (messages_day_changed(last, msg->time)) {
            MSG_HEADER *notice = messages_create_day_notice(msg->time);
            if (notice) {
                expanded[out++] = notice;
            }
        }
        expanded[out++] = msg;
        last = msg->time;
    }

    if (out > 0 && m->number > 0 && m->data[0]
        && messages_day_changed(last, m->data[0]->time)) {
        MSG_HEADER *notice = messages_create_day_notice(m->data[0]->time);
        if (notice) {
            expanded[out++] = notice;
        }
    }

    if (out == 0) {
        free(expanded);
        pthread_mutex_unlock(&messages_lock);
        return 0;
    }

    if (out > UTOX_MAX_BACKLOG_MESSAGES) {
        LOG_ERR("Messages", "Prepend batch larger than backlog cap; refusing.");
        for (size_t i = 0; i < out; i++) {
            if (expanded[i] && expanded[i]->msg_type == MSG_TYPE_NOTICE_DAY_CHANGE) {
                message_free(expanded[i]);
            }
        }
        free(expanded);
        pthread_mutex_unlock(&messages_lock);
        return 0;
    }

    while (m->number + out > UTOX_MAX_BACKLOG_MESSAGES && m->number > 0) {
        messages_drop_newest(m);
    }

    const int    old_height = m->height;
    const size_t need       = m->number + out;

    MSG_HEADER **ndata = realloc(m->data, (need + 10) * sizeof(MSG_HEADER *));
    if (!ndata) {
        LOG_ERR("Messages", "Failed to realloc for prepend.");
        for (size_t i = 0; i < out; i++) {
            if (expanded[i] && expanded[i]->msg_type == MSG_TYPE_NOTICE_DAY_CHANGE) {
                message_free(expanded[i]);
            }
        }
        free(expanded);
        pthread_mutex_unlock(&messages_lock);
        return 0;
    }

    m->data  = ndata;
    m->extra = 10;

    memmove(m->data + out, m->data, m->number * sizeof(MSG_HEADER *));
    memcpy(m->data, expanded, out * sizeof(MSG_HEADER *));
    m->number += (uint32_t)out;
    messages_bump_indices_for_prepend(m, out);

    if (m->width != 0) {
        messages_updateheight(m, m->width);
        messages_adjust_scroll_after_prepend(m, old_height);
    }

    free(expanded);
    pthread_mutex_unlock(&messages_lock);
    return out;
}

bool messages_load_older_chatlog(MESSAGES *m, const char *hex) {
    if (!m || !hex || m->is_groupchat || m->chatlog_exhausted || m->chatlog_loading) {
        return false;
    }

    const size_t total = utox_count_chatlog((char *)hex);
    if (m->chatlog_skip >= total) {
        m->chatlog_exhausted = true;
        return false;
    }

    m->chatlog_loading = true;

    size_t       n     = 0;
    MSG_HEADER **batch = utox_load_chatlog((char *)hex, &n, UTOX_CHATLOG_PAGE_SIZE, m->chatlog_skip);
    if (!batch || n == 0) {
        if (batch) {
            for (size_t i = 0; batch[i]; i++) {
                message_free(batch[i]);
            }
            free(batch);
        }
        m->chatlog_exhausted = true;
        m->chatlog_loading   = false;
        return false;
    }

    const size_t prepended = messages_prepend(m, batch, n);
    if (prepended == 0) {
        for (size_t i = 0; i < n; i++) {
            message_free(batch[i]);
        }
        free(batch);
        m->chatlog_exhausted = true;
        m->chatlog_loading   = false;
        return false;
    }

    m->chatlog_skip += (uint32_t)n;
    if (n < UTOX_CHATLOG_PAGE_SIZE || m->chatlog_skip >= total) {
        m->chatlog_exhausted = true;
    }

    free(batch);
    m->chatlog_loading = false;
    return true;
}
