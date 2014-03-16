
enum
{
    CMSG_SETNAME,
    CMSG_SETSTATUSMSG,
    CMSG_ADDFRIEND,
    CMSG_DELFRIEND,
    CMSG_ACCEPTFRIEND,
    CMSG_SENDMESSAGE,
    CMSG_SENDMESSAGEGROUP,
    CMSG_NEWGROUP,
    CMSG_LEAVEGROUP,
    CMSG_GROUPINVITE,
};

void core_postmessage(int msg, int param, uint32_t len, void *data);
void core_postmessage2(int msg, int param, uint32_t len, void *data, void *data2);
void core_postmessage3(int msg, int param);

void core_thread(void *args);
