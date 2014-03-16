
#include "main.h"

FRIENDREQ** newfriendreq(uint8_t *id)
{
    int i = 0;
    while(i < requests)
    {
        if(memcmp(request[i]->id, id, TOX_CLIENT_ID_SIZE) == 0)
        {
            return NULL;
        }
        i++;
    }

    return &request[requests++];
}
