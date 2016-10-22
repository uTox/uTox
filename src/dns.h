#ifndef DNS_H
#define DNS_H

#include <stddef.h>

enum {
    ADDF_NONE,
    ADDF_SENT,
    ADDF_DISCOVER,
    ADDF_BADNAME,
    ADDF_NONAME,
    ADDF_TOOLONG,      // if message length is too long.
    ADDF_NOMESSAGE,    // if no message (message length must be >= 1 byte).
    ADDF_OWNKEY,       // if user's own key.
    ADDF_ALREADYSENT,  // if friend request already sent or already a friend.
    ADDF_UNKNOWN,      // for unknown error.
    ADDF_BADCHECKSUM,  // if bad checksum in address.
    ADDF_SETNEWNOSPAM, // if the friend was already there but the nospam was different.
    ADDF_NOMEM,        // if increasing the friend list size fails.
};

void dns_request(char *name, size_t length);

#endif
