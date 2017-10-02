#include "group_invite.h"
#include "groups.h"
#include "utox.h"

#include <stdlib.h>
#include <string.h>

#include <tox/toxav.h>

#define MAX_GROUP_INVITES 16

struct group_invite {
    uint32_t friend_number;
    uint8_t *cookie;
    size_t length;
    bool is_av_group;
};

static struct group_invite *invites[MAX_GROUP_INVITES];

static uint8_t find_free_slot(void) {
    for (uint8_t i = 0; i < MAX_GROUP_INVITES; ++i) {
        if (!invites[i]) {
            return i;
        }
    }
    return UINT8_MAX;
}

static void group_invite_free(const uint8_t invite_id) {
    free(invites[invite_id]->cookie);
    free(invites[invite_id]);
    invites[invite_id] = NULL;
}

uint8_t group_invite_new(const uint32_t friend_number,
                         const uint8_t *cookie,
                         const size_t cookie_length,
                         const bool is_av_group)
{
    const uint8_t invite_id = find_free_slot();
    if (invite_id == UINT8_MAX) {
        return UINT8_MAX;
    }

    struct group_invite *invite = calloc(1, sizeof(struct group_invite));
    if (!invite) {
        return UINT8_MAX;
    }

    invite->friend_number = friend_number;
    invite->length = cookie_length;
    invite->is_av_group = is_av_group;
    invite->cookie = calloc(cookie_length, sizeof(uint8_t));
    memcpy(invite->cookie, cookie, cookie_length * sizeof(uint8_t));

    invites[invite_id] = invite;

    return invite_id;
}

// Yeah, this is ugly and hacky, but to be fair, I only moved it here from tox_callbacks.c.
void callback_av_group_audio(void *tox, int groupnumber, int peernumber, const int16_t *pcm,
                             unsigned int samples, uint8_t channels, unsigned int sample_rate,
                             void *userdata);

bool group_invite_accept(Tox *tox, const uint8_t invite_id) {
    uint32_t group_id = UINT32_MAX;

    if (invites[invite_id]->is_av_group) {
        group_id = toxav_join_av_groupchat(tox,
                                           invites[invite_id]->friend_number,
                                           invites[invite_id]->cookie,
                                           invites[invite_id]->length,
                                           callback_av_group_audio,
                                           NULL);
    } else {
        group_id = tox_conference_join(tox,
                                       invites[invite_id]->friend_number,
                                       invites[invite_id]->cookie,
                                       invites[invite_id]->length,
                                       NULL);
    }

    if (group_id == UINT32_MAX) {
        group_invite_free(invite_id);
        return false;
    }

    GROUPCHAT *groupchat = get_group(group_id);
    if (groupchat) {
        group_init(groupchat, group_id, invites[invite_id]->is_av_group);
    } else {
        group_create(group_id, invites[invite_id]->is_av_group);
    }

    group_invite_free(invite_id);

    postmessage_utox(GROUP_ADD, group_id, 0, tox);
    return true;
}

void group_invite_reject(const uint8_t invite_id) {
    group_invite_free(invite_id);
}

uint32_t group_invite_get_friend_id(const uint8_t invite_id) {
    return invites[invite_id]->friend_number;
}
