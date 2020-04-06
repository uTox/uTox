#include "self.h"

#include "avatar.h"
#include "debug.h"
#include "qr.h"
#include "tox.h"

#include "ui/edit.h"
#include "layout/settings.h"
#include "native/filesys.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct utox_self self;

void init_self(Tox *tox) {
    /* Set local info for self */
    edit_setstr(&edit_name, self.name, self.name_length);
    edit_setstr(&edit_status_msg, self.statusmsg, self.statusmsg_length);

    /* Get tox id, and gets the hex version for utox */
    tox_self_get_address(tox, self.id_binary);
    id_to_string(self.id_str, self.id_binary);
    self.id_str_length = TOX_ADDRESS_SIZE * 2;
    LOG_TRACE("Self INIT", "Tox ID: %.*s" , (int)self.id_str_length, self.id_str);

    qr_setup(self.id_str, &self.qr_data, &self.qr_data_size, &self.qr_image, &self.qr_image_size);

    /* Get nospam */
    self.nospam = tox_self_get_nospam(tox);
    self.old_nospam = self.nospam;
    sprintf(self.nospam_str, "%08X", self.nospam);
    edit_setstr(&edit_nospam, self.nospam_str, sizeof(uint32_t) * 2);

    avatar_init_self();
}
