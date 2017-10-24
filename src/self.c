#include "self.h"

#include "avatar.h"
#include "debug.h"
#include "tox.h"

#include "ui/edit.h"
#include "layout/settings.h"

#include <stdio.h>

void init_self(Tox *tox) {
    /* Set local info for self */
    edit_setstr(&edit_name, self.name, self.name_length);
    edit_setstr(&edit_status_msg, self.statusmsg, self.statusmsg_length);

    /* Get tox id, and gets the hex version for utox */
    tox_self_get_address(tox, self.id_binary);
    id_to_string(self.id_str, self.id_binary);
    self.id_str_length = TOX_ADDRESS_SIZE * 2;
    LOG_TRACE("Self INIT", "Tox ID: %.*s" , (int)self.id_str_length, self.id_str);

    /* Get nospam */
    self.nospam = tox_self_get_nospam(tox);
    self.old_nospam = self.nospam;
    sprintf(self.nospam_str, "%08X", self.nospam);
    edit_setstr(&edit_nospam, self.nospam_str, sizeof(uint32_t) * 2);

    avatar_init_self();
}

TOX_USER_STATUS user_status_to_tox(USER_STATUS utox_user_status) {
    if (utox_user_status == USER_STATUS_OFFLINE ||
        utox_user_status == USER_STATUS_INVALID) {
        return TOX_USER_STATUS_NONE;
    }

    return (TOX_USER_STATUS)(utox_user_status - 1);
}

USER_STATUS user_status_to_utox(TOX_USER_STATUS tox_user_status) {
    return (USER_STATUS)(tox_user_status + 1);
}
