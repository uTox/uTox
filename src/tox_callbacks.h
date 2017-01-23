#ifndef TOX_CALLBACKS_H
#define TOX_CALLBACKS_H

#include <tox/tox.h>

void utox_set_callbacks_friends(Tox *tox);
void utox_set_callbacks_groups(Tox *tox);
void utox_set_callbacks_mdevice(Tox *tox);

#endif
