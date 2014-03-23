
#include "main.h"

typedef struct
{
    uint16_t msg, param;
    uint32_t len;
    uint8_t data[1016];
}CMSG;

static CMSG core_msg;

static void callback_friend_request(Tox *tox, uint8_t *id, uint8_t *msg, uint16_t length, void *userdata)
{
    void *data = malloc(2 + TOX_FRIEND_ADDRESS_SIZE + length);
    *(uint16_t*)data = length;
    memcpy(data + 2, id, TOX_FRIEND_ADDRESS_SIZE);
    memcpy(data + 2 + TOX_FRIEND_ADDRESS_SIZE, msg, length);

    PostMessage(hwnd, WM_FREQUEST, 0, (LPARAM)data);

    printf("Friend Request: %.*s\n", length, msg);
}

static void callback_friend_message(Tox *tox, int fid, uint8_t *message, uint16_t length, void *userdata)
{
    if(message[length - 1] == 0)
    {
        printf("nullterm\n");
        length--;
    }

    uint16_t *data = malloc(length + 4);
    data[0] = 1;
    data[1] = length;
    memcpy((void*)data + 4, message, length);

    PostMessage(hwnd, WM_FMESSAGE, fid, (LPARAM)data);

    printf("Friend Message (%u): %.*s\n", fid, length, message);
}

static void callback_friend_action(Tox *tox, int fid, uint8_t *action, uint16_t length, void *userdata)
{
    if(action[length - 1] == 0)
    {
        length--;
    }

    uint16_t *data = malloc(length + 4);
    data[0] = 3;
    data[1] = length;
    memcpy((void*)data + 4, action, length);

    PostMessage(hwnd, WM_FACTION, fid, (LPARAM)data);

    printf("Friend Action (%u): %.*s\n", fid, length, action);
}

static void callback_name_change(Tox *tox, int fid, uint8_t *newname, uint16_t length, void *userdata)
{
    void *data = malloc(length);
    memcpy(data, newname, length);

    PostMessage(hwnd, WM_FNAME, (fid << 16 | length), (LPARAM)data);

    printf("Friend Name (%u): %.*s\n", fid, length, newname);
}

static void callback_status_message(Tox *tox, int fid, uint8_t *newstatus, uint16_t length, void *userdata)
{
    length--;

    void *data = malloc(length);
    memcpy(data, newstatus, length);

    PostMessage(hwnd, WM_FSTATUSMSG, (fid << 16 | length), (LPARAM)data);

    printf("Friend Status Message (%u): %.*s\n", fid, length, newstatus);
}

static void callback_user_status(Tox *tox, int fid, uint8_t status, void *userdata)
{
    PostMessage(hwnd, WM_FSTATUS, fid, status);

    printf("Friend Userstatus (%u): %u\n", fid, status);
}

static void callback_typing_change(Tox *tox, int fid, uint8_t is_typing, void *userdata)
{
    PostMessage(hwnd, WM_FTYPING, fid, is_typing);

    printf("Friend Typing (%u): %u\n", fid, is_typing);
}

static void callback_read_receipt(Tox *tox, int fid, uint32_t receipt, void *userdata)
{
    PostMessage(hwnd, WM_FRECEIPT, fid, receipt);

    printf("Friend Receipt (%u): %u\n", fid, receipt);
}

static void callback_connection_status(Tox *tox, int fid, uint8_t status, void *userdata)
{
    PostMessage(hwnd, WM_FONLINE, fid, status);

    printf("Friend Online/Offline (%u): %u\n", fid, status);
}

static void callback_group_invite(Tox *tox, int friendnumber, uint8_t *group_public_key, void *userdata)
{
    int g = tox_join_groupchat(tox, friendnumber, group_public_key);

    if(g >= 0)
    {
        PostMessage(hwnd, WM_GADD, g, 0);

        printf("Group Joined (%u,f:%u)\n", g, friendnumber);
    }
}

static void callback_group_message(Tox *tox, int groupnumber, int friendgroupnumber, uint8_t *message, uint16_t length, void *userdata)
{
    if(message[length - 1] == 0)
    {
        length--;
    }

    uint8_t name[TOX_MAX_NAME_LENGTH];
    int namelen = tox_group_peername(tox, groupnumber, friendgroupnumber, name);
    if(namelen == 0 || namelen == -1)
    {
        memcpy(name, "<unknown>", 9);
        namelen = 9;
    }

    uint16_t *data = malloc(namelen + length + 4);
    data[0] = (friendgroupnumber << 9) | namelen;
    data[1] = length;
    memcpy((void*)data + 4, message, length);
    memcpy((void*)data + 4 + length, name, namelen);

    PostMessage(hwnd, WM_GMESSAGE, groupnumber, (LPARAM)data);

    printf("Group Message (%u, %u): %s\n", groupnumber, friendgroupnumber, message);
}

static void callback_group_action(Tox *tox, int groupnumber, int friendgroupnumber, uint8_t *action, uint16_t length, void *userdata)
{
    if(action[length - 1] == 0)
    {
        length--;
    }

    uint8_t name[TOX_MAX_NAME_LENGTH];
    int namelen = tox_group_peername(tox, groupnumber, friendgroupnumber, name);
    if(namelen == 0)
    {
        memcpy(name, "<unknown>", 9);
        namelen = 9;
    }

    uint16_t *data = malloc(namelen + length + 4);
    data[0] = (friendgroupnumber << 9) | 0x100 | namelen;
    data[1] = length;
    memcpy((void*)data + 4, action, length);
    memcpy((void*)data + 4 + length, name, namelen);

    PostMessage(hwnd, WM_GACTION, groupnumber, (LPARAM)data);

    printf("Group Action (%u, %u): %s\n", groupnumber, friendgroupnumber, action);
}

static void callback_group_namelist_change(Tox *tox, int groupnumber, int peernumber, uint8_t change, void *userdata)
{
    int param = ((groupnumber << 16) | peernumber);
    switch(change)
    {
        case TOX_CHAT_CHANGE_PEER_ADD:
        {
            PostMessage(hwnd, WM_GPEERADD, param, 0);
            break;
        }

        case TOX_CHAT_CHANGE_PEER_DEL:
        {
            PostMessage(hwnd, WM_GPEERDEL, param, 0);
            break;
        }

        case TOX_CHAT_CHANGE_PEER_NAME:
        {
            uint8_t name[TOX_MAX_NAME_LENGTH];
            int len = tox_group_peername(tox, groupnumber, peernumber, name);

            uint8_t *data = malloc(len + 1);
            data[0] = len;
            memcpy(data + 1, name, len);

            PostMessage(hwnd, WM_GPEERNAME, param, (LPARAM)data);
            break;
        }
    }
    //printf("Group Namelist Change (%u, %u): %u\n", groupnumber, peernumber, change);
}

static void dobootstrap(Tox *tox)
{
    //bootstrap with next 2 nodes in list

    static int i = 0;
    int k = 0;
    while(k < 1)
    {
        uint8_t *d = bootstrap_nodes[i++];
        if(i >= countof(bootstrap_nodes))
        {
            i = 0;
        }
        tox_IP4 ip4 = {.c = {d[0], d[1], d[2], d[3]}};
        tox_IP_Port ip_port = {.ip = {.family = AF_INET}, .port = (d[4] | d[5] << 8)};
        ip_port.ip.ip4 = ip4;

        tox_bootstrap_from_ip(tox, ip_port, d + 6);

        k++;
    }
}

void core_postmessage(int msg, int param, uint32_t len, void *data)
{
    len = (len > sizeof(core_msg.data)) ? sizeof(core_msg.data) : len;

    while(tox_thread_msg){Sleep(1);}

    core_msg.msg = msg;
    core_msg.param = param;
    core_msg.len = len;
    memcpy(core_msg.data, data, len);

    tox_thread_msg = 1;
}

void core_postmessage2(int msg, int param, uint32_t len, void *data, void *data2)
{
    while(tox_thread_msg){Sleep(1);}

    core_msg.msg = msg;
    core_msg.param = param;
    core_msg.len = len + param;
    memcpy(core_msg.data, data, len);
    memcpy(core_msg.data + len, data2, param);

    tox_thread_msg = 1;
}

void core_postmessage3(int msg, int param)
{
    while(tox_thread_msg){Sleep(1);}

    core_msg.msg = msg;
    core_msg.param = param;
    core_msg.len = param >> 16;

    tox_thread_msg = 1;
}

void core_thread(void *args)
{
    Tox *tox;
    FILE *file;
    void *data = NULL;
    int len;

    file = fopen(SAVE_NAME, "rb");
    if(file)
    {
        fseek(file, 0, SEEK_END);
        len = ftell(file);
        fseek(file, 0, SEEK_SET);

        data = malloc(len);
        if(fread(data, len, 1, file) == 1)
        {
            printf("loaded tox.data (%u)\n", len);
        }
        else
        {
            free(data);
            data = NULL;
        }

        fclose(file);
    }

    tox = tox_new(0);

    if(data)
    {
        int l = *(uint32_t*)data;
        int r = tox_load(tox, data + 4, l);

        if(!r)
        {
            void *d = data + (4 + l);
            len -= (4 + l);

            while(len != 0)
            {
                FRIENDREQ **f = &request[requests++];
                uint16_t length;
                memcpy(&length, d, 2);

                l = (length + TOX_FRIEND_ADDRESS_SIZE + 2);
                *f = malloc(l);
                memcpy(*f, d, l);
                d += l;
                len -= l;
            }
        }

        //printf("reqs loaded: %u\n", requests);

        free(data);
        if(r != 0)
        {
            goto INVALID_SAVE;
        }

        friends = tox_count_friendlist(tox);

        uint32_t i = 0;
        while(i != friends)
        {
            int size;
            FRIEND *f = &friend[i];
            uint8_t name[128];

            size = tox_get_name(tox, i, name);
            f->name = malloc(size);
            memcpy(f->name, name, size);
            f->name_length = size;

            size = tox_get_status_message_size(tox, i);
            f->status_message = malloc(size);
            tox_get_status_message(tox, i, f->status_message, size);
            f->status_length = size;

            i++;
        }
    }
    else
    {
        INVALID_SAVE:
        printf("no valid save file, using defaults\n");

        tox_set_name(tox, (uint8_t*)"Anon", 5);
        tox_set_status_message(tox, (uint8_t*)"Toxing", 7);
        tox_set_user_status(tox, TOX_USERSTATUS_AWAY);

        dobootstrap(tox);
    }

    name_length = tox_get_self_name(tox, name);
    status_length = tox_get_self_status_message(tox, statusmsg, sizeof(statusmsg));//do something if length of status message > sizeof(statusmsg)

    //until tox doesnt require null characters to be included in string length
    name_length--;
    status_length--;

    memcpy(edit_name_data, name, name_length);
    edit_name.length = name_length;

    int slen = (status_length > sizeof(edit_status_data)) ? sizeof(edit_status_data) : status_length;
    memcpy(edit_status_data, statusmsg, slen);
    edit_status.length = slen;

    uint8_t id[TOX_FRIEND_ADDRESS_SIZE];
    tox_get_address(tox, id);
    sprint_address(tox_address_string, id);

    status = tox_get_self_user_status(tox);



    tox_callback_friend_request(tox, callback_friend_request, NULL);
    tox_callback_friend_message(tox, callback_friend_message, NULL);
    tox_callback_friend_action(tox, callback_friend_action, NULL);
    tox_callback_name_change(tox, callback_name_change, NULL);
    tox_callback_status_message(tox, callback_status_message, NULL);
    tox_callback_user_status(tox, callback_user_status, NULL);
    tox_callback_typing_change(tox, callback_typing_change, NULL);
    tox_callback_read_receipt(tox, callback_read_receipt, NULL);
    tox_callback_connection_status(tox, callback_connection_status, NULL);

    tox_callback_group_invite(tox, callback_group_invite, NULL);
    tox_callback_group_message(tox, callback_group_message, NULL);
    tox_callback_group_action(tox, callback_group_action, NULL);
    tox_callback_group_namelist_change(tox, callback_group_namelist_change, NULL);

    tox_thread_b = 1;

    _Bool connected = 0;
    clock_t last_do = clock(), last_bootstrap = last_do;

    while(tox_thread_b)
    {
        //tox_do approximately 20 times per sec
        if(clock() - last_do >= CLOCKS_PER_SEC / 20)
        {
            last_do += CLOCKS_PER_SEC / 20;
            tox_do(tox);
        }

        if(!connected)
        {
            if(tox_isconnected(tox))
            {
                connected = 1;
                printf("Now CONNECTED!\n");

                //send an event
            }
            else
            {
                //try bootstrap approximately every 2 seconds
                clock_t now = clock();
                if(now - last_bootstrap >= CLOCKS_PER_SEC * 2)
                {

                    last_bootstrap = now;

                    dobootstrap(tox);
                }
            }
        }

        if(tox_thread_msg)
        {
            CMSG *msg = &core_msg;

            switch(msg->msg)
            {
                case CMSG_SETNAME:
                {
                    tox_set_name(tox, msg->data, msg->len);
                    break;
                }

                case CMSG_SETSTATUSMSG:
                {
                    tox_set_status_message(tox, msg->data, msg->len);
                    break;
                }

                case CMSG_ADDFRIEND:
                {
                    int r = tox_add_friend(tox, msg->data, msg->data + TOX_FRIEND_ADDRESS_SIZE, msg->param);
                    void *rp = malloc(TOX_FRIEND_ADDRESS_SIZE);
                    memcpy(rp, msg->data, TOX_FRIEND_ADDRESS_SIZE);

                    PostMessage(hwnd, WM_FADD, r, (LPARAM)rp);
                    break;
                }

                case CMSG_DELFRIEND:
                {
                    tox_del_friend(tox, msg->param);
                    break;
                }

                case CMSG_ACCEPTFRIEND:
                {
                    int r = tox_add_friend_norequest(tox, msg->data);
                    void *rdata = malloc(msg->len);
                    memcpy(rdata, msg->data, msg->len);

                    PostMessage(hwnd, WM_FACCEPT, r, (LPARAM)rdata);
                    break;
                }

                case CMSG_SENDMESSAGE:
                {
                    tox_send_message(tox, msg->param, msg->data, msg->len);
                    break;
                }

                case CMSG_SENDMESSAGEGROUP:
                {
                    tox_group_message_send(tox, msg->param, msg->data, msg->len);
                    break;
                }

                case CMSG_NEWGROUP:
                {
                    int g = tox_add_groupchat(tox);
                    if(g != -1)
                    {
                        PostMessage(hwnd, WM_GADD, g, 0);
                    }

                    break;
                }

                case CMSG_LEAVEGROUP:
                {
                    tox_del_groupchat(tox, msg->param);
                    break;
                }

                case CMSG_GROUPINVITE:
                {
                    tox_invite_friend(tox, msg->len, msg->param);
                    break;
                }
            }

            tox_thread_msg = 0;
        }
        Sleep(1);
    }

    len = tox_size(tox);
    data = malloc(len + 4);

    *(uint32_t*)data = len;
    tox_save(tox, data + 4);

    file = fopen(SAVE_NAME, "wb");
    if(file)
    {
        fwrite(data, len + 4, 1, file);

        int i = 0;
        while(i < requests)
        {
            fwrite(request[i], request[i]->length + TOX_FRIEND_ADDRESS_SIZE + 2, 1, file);
            i++;
        }

        fclose(file);
    }

    free(data);

    tox_kill(tox);

    tox_thread_b = 1;
}
