#ifndef MOCK_DOMAIN_H
#define MOCK_DOMAIN_H

#include "../../src/flist.h"
#include "../../src/friend.h"
#include "../../src/groups.h"
#include "../../src/utox.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tox/tox.h>

extern FRIEND *mock_sel_friend;
extern GROUPCHAT *mock_sel_group;
extern ITEM_TYPE mock_sel_item_type;

extern uint8_t mock_last_tox_msg;
extern uint32_t mock_last_tox_p1;
extern uint32_t mock_last_tox_p2;
extern void *mock_last_tox_data;
extern uint8_t mock_last_tox_blob[TOX_ADDRESS_SIZE];
extern size_t mock_last_tox_blob_len;

extern uint8_t mock_last_audio_msg;
extern uint32_t mock_last_audio_param1;

extern char mock_last_openurl[512];

extern uint32_t mock_tox_friend_count;
extern uint32_t mock_tox_conference_count;
extern char mock_tox_conference_title[64];
/* When non-zero, tox_conference_get_title_size reports this error instead of OK. */
extern int mock_tox_conference_title_err;

extern UTOX_MSG mock_last_utox_msg;
extern uint16_t mock_last_utox_p1;
extern uint16_t mock_last_utox_p2;
extern void *mock_last_utox_data;

extern uint32_t mock_tox_file_send_next;
extern uint32_t mock_last_file_kind;
extern uint64_t mock_last_file_size;
extern uint8_t mock_last_file_hash[TOX_HASH_LENGTH];
extern uint8_t mock_incoming_file_id[TOX_HASH_LENGTH];
extern uint8_t mock_last_filename[256];
extern size_t mock_last_filename_len;
extern TOX_ERR_FILE_SEND mock_tox_file_send_err;
extern bool mock_tox_file_control_ok;
extern TOX_FILE_CONTROL mock_last_file_control;
extern uint32_t mock_file_control_count;
extern uint32_t mock_file_send_chunk_count;
extern uint64_t mock_last_chunk_position;
extern size_t mock_last_chunk_length;

extern tox_file_recv_cb *mock_cb_file_recv;
extern tox_file_recv_control_cb *mock_cb_file_recv_control;
extern tox_file_recv_chunk_cb *mock_cb_file_recv_chunk;
extern tox_file_chunk_request_cb *mock_cb_file_chunk_request;

extern tox_friend_request_cb *mock_cb_friend_request;
extern tox_friend_message_cb *mock_cb_friend_message;
extern tox_friend_name_cb *mock_cb_friend_name;
extern tox_friend_status_message_cb *mock_cb_friend_status_message;
extern tox_friend_status_cb *mock_cb_friend_status;
extern tox_friend_typing_cb *mock_cb_friend_typing;
extern tox_friend_read_receipt_cb *mock_cb_friend_read_receipt;
extern tox_friend_connection_status_cb *mock_cb_friend_connection_status;

extern tox_conference_invite_cb *mock_cb_conference_invite;
extern tox_conference_message_cb *mock_cb_conference_message;
extern tox_conference_peer_name_cb *mock_cb_conference_peer_name;
extern tox_conference_title_cb *mock_cb_conference_title;
extern tox_conference_peer_list_changed_cb *mock_cb_conference_peer_list_changed;
extern tox_conference_connected_cb *mock_cb_conference_connected;

extern uint32_t mock_tox_conference_join_result;
extern bool mock_tox_conference_join_fail;
extern int32_t mock_toxav_join_result;
extern uint32_t mock_tox_conference_peer_count_n;
extern char mock_tox_conference_peer_name[TOX_MAX_NAME_LENGTH];
extern size_t mock_tox_conference_peer_name_len;
extern size_t mock_tox_conference_peer_name_size_report;
extern uint8_t mock_tox_conference_peer_pk[TOX_PUBLIC_KEY_SIZE];

void mock_domain_reset(void);

/* Controllable clock for get_time(); 0 is the default (falsy last_check_time). */
void mock_time_set(uint64_t now);

#endif
