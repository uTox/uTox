#include "main.h"

void friend_setname(FRIEND *f, uint8_t *name, uint16_t length)
{
    free(f->name);
    if(length == 0) {
        f->name = malloc(sizeof(f->cid) * 2);
        cid_to_string(f->name, f->cid);
        f->name_length = sizeof(f->cid) * 2;
    } else {
        f->name = malloc(length);
        memcpy(f->name, name, length);
        f->name_length = length;
    }
}

FILE_T* friend_newincoming(FRIEND *f, uint8_t filenumber)
{
    if(filenumber >= f->nincoming)
    {
        f->nincoming = filenumber + 1;
        FILE_T *new = realloc(f->incoming, f->nincoming * sizeof(FILE_T));
        if(!new)
        {
            return NULL;
        }

        f->incoming = new;
    }

    FILE_T *ft = &f->incoming[filenumber];
    memset(ft, 0, sizeof(FILE_T));
    return ft;
}

FILE_T* friend_newoutgoing(FRIEND *f, uint8_t filenumber)
{
    if(filenumber >= f->noutgoing)
    {
        f->noutgoing = filenumber + 1;
        FILE_T *new = realloc(f->outgoing, f->noutgoing * sizeof(FILE_T));
        if(!new)
        {
            return NULL;
        }

        f->outgoing = new;
    }

    FILE_T *ft = &f->outgoing[filenumber];
    memset(ft, 0, sizeof(FILE_T));
    return ft;
}
