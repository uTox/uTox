#ifndef LAYOUT_GROUP_H
#define LAYOUT_GROUP_H

#include "../ui/draw.h"
#include "../ui/panel.h"
#include "../ui/scrollable.h"

#include "../flist.h"
#include "../groups.h"
#include "../macros.h"
#include "../settings.h"
#include "../theme.h"

#include "../ui/svg.h"

#include <stddef.h>
#include <stdio.h>
#include <tox/tox.h>

extern SCROLLABLE scrollbar_group;

extern PANEL panel_group,
                panel_group_chat,
                panel_group_video,
                panel_group_settings,
                messages_group;

#endif // LAYOUT_GROUP_H
