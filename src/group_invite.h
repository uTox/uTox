#ifndef GROUP_INVITE_H
#define GROUP_INVITE_H

#include <tox/tox.h>

#include <stdbool.h>
#include <stdint.h>

// Returns a new GROUP_REQUEST or NULL on failure. Caller assumes ownership.
uint8_t group_invite_new(const uint32_t friend_number,
                         const uint8_t *cookie,
                         const size_t length,
                         const bool is_av_group);

// Returns a bool indicating whether the group invite was accepted successfully.
// Calling this function frees the GROUP_REQUEST passed.
bool group_invite_accept(Tox *tox, const uint8_t request_id);
void group_invite_reject(const uint8_t request_id);

uint32_t group_invite_get_friend_id(const uint8_t request_id);

#endif
