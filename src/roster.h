/* list: the contact list
 */

// call to switch to previous or next friend in list
void previous_tab(void);
void next_tab(void);

// update the shown list, should be called whenever something relevant to the filters is done
// (like changing name, going online, etc.)
void update_shown_list(void);

// set or get current list filter. Updates list afterwards
uint8_t list_get_filter(void);
void list_set_filter(uint8_t filter);

// set the search string in the list. Disable search by setting it to NULL. Updates list afterwards
// warning: list will just remember the pointer, it will assume you won't deallocate the memory, and it
// won't deallocate it after setting to NULL. The string should be NULL-terminated.
void list_search(char_t *str);



/* non-exhaustive list of panels we to select from, it's probably better to replace this but I don't know with what. */
enum
{
    ITEM_NONE,
    ITEM_ADD,
    ITEM_SETTINGS,
    ITEM_FRIEND,
    ITEM_GROUP,
    ITEM_CREATE_GROUP,
    ITEM_FRIEND_ADD,
};

typedef struct
{
    uint8_t item;
    char namestr[15];
    void *data;
}ITEM;

extern ITEM *selected_item;
ITEM *right_mouse_item;

void list_start(void);
void list_addfriend(FRIEND *f);
void list_addfriend2(FRIEND *f, FRIENDREQ *req);
void list_addgroup(GROUPCHAT *g);
void list_addfriendreq(FRIENDREQ *f);
void list_deletesitem(void);
void list_deleteright_mouse_item(void);

void list_selectchat(int index);
void list_selectaddfriend(void);
void list_reselect_current(void);
void list_selectsettings(void);
void list_selectswap(void);

void roster_re_scale(void);

void list_draw(void *n, int x, int y, int width, int height);
void list_freeall(void);

_Bool list_mmove(void *n, int x, int y, int width, int height, int mx, int my, int dx, int dy);
_Bool list_mdown(void *n);
_Bool list_mright(void *n);
_Bool list_mwheel(void *n, int height, double d, _Bool smooth);
_Bool list_mup(void *n);
_Bool list_mleave(void *n);

/* New naming patten */
void roster_select_last(void);
