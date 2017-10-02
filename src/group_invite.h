#ifndef GROUP_INVITE_H
#define GROUP_INVITE_H

#include <tox/tox.h>

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Create new group invite.
 *
 * @param friend_number Friend number from contact list who's inviting.
 * @param cookie Unique groupchat id.
 * @param cookie_length Groupchat id's length.
 * @param is_av_group Is this groupchat text-only or not.
 * @return New id of created invite or UINT8_MAX on failure.
 */
uint8_t group_invite_new(const uint32_t friend_number,
                         const uint8_t *cookie,
                         const size_t cookie_length,
                         const bool is_av_group);

/**
 * @brief Accept invite into groupchat.
 *
 * Calling this function frees the GROUP_INVITE passed.
 *
 * @return whether the group invite was accepted successfully.
 */
bool group_invite_accept(Tox *tox, const uint8_t invite_id);
void group_invite_reject(const uint8_t invite_id);

/**
 * @brief Get friend who's inviting.
 */
uint32_t group_invite_get_friend_id(const uint8_t invite_id);

#endif
