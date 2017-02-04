#ifndef FLIST_H
#define FLIST_H

#include <stdbool.h>
#include <stdint.h>

typedef struct groupchat GROUPCHAT;
typedef struct utox_friend FRIEND;
typedef struct utox_friend_request FRIENDREQ;

// call to switch to previous or next friend in list
void flist_previous_tab(void);
void flist_next_tab(void);

// update the shown list, should be called whenever something relevant to the filters is done
// (like changing name, going online, etc.)
void flist_update_shown_list(void);

// set or get current list filter. Updates list afterwards
uint8_t flist_get_filter(void);
void flist_set_filter(uint8_t filter);

// set the search string in the list. Disable search by setting it to NULL. Updates list afterwards
// warning: list will just remember the pointer, it will assume you won't deallocate the memory, and it
// won't deallocate it after setting to NULL. The string should be NULL-terminated.
void flist_search(char *str);

/* non-exhaustive list of panels we to select from, it's probably better to replace this but I don't know with what. */
typedef enum {
    ITEM_NONE,
    ITEM_SETTINGS,
    ITEM_ADD,
    ITEM_FRIEND,
    ITEM_FRIEND_ADD,
    ITEM_GROUP,
    ITEM_GROUP_CREATE,
} ITEM_TYPE;

typedef struct {
    ITEM_TYPE item;
    char      namestr[15];
    void *    data;
} ITEM;

ITEM *right_mouse_item;

void flist_start(void);
void flist_addfriend(FRIEND *f);
void flist_addfriend2(FRIEND *f, FRIENDREQ *req);
void flist_addgroup(GROUPCHAT *g);
void flist_addfriendreq(FRIENDREQ *f);
void flist_deletesitem(void);
void flist_delete_rmouse_item(void);

void flist_selectchat(int index);
void flist_selectaddfriend(void);
void flist_reselect_current(void);
void flist_selectsettings(void);
void flist_selectswap(void);

void flist_re_scale(void);

void flist_freeall(void);

/* New naming patten */
void flist_select_last(void);
void flist_dump_contacts(void);
void flist_reload_contacts(void);

ITEM *flist_get_selected(void);

/* UI functions */
void flist_draw(void *n, int x, int y, int width, int height);
bool flist_mmove(void *n, int x, int y, int width, int height, int mx, int my, int dx, int dy);
bool flist_mdown(void *n);
bool flist_mright(void *n);
bool flist_mwheel(void *n, int height, double d, bool smooth);
bool flist_mup(void *n);
bool flist_mleave(void *n);

#endif
