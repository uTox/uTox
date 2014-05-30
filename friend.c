#include "main.h"

void friend_setname(FRIEND *f, uint8_t *name, uint16_t length)
{
    free(f->name);
    if(length == 0)
    {
        f->name = malloc(sizeof(f->cid) * 2);
        cid_to_string(f->name, f->cid);
        f->name_length = sizeof(f->cid) * 2;
    }
    else
    {
        f->name = malloc(length);
        memcpy(f->name, name, length);
        f->name_length = length;
    }
}
