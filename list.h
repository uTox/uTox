/* list: the contact list
 */

enum
{
    ITEM_NONE,
    ITEM_ADD,
    ITEM_SETTINGS,
    ITEM_TRANSFER,
    ITEM_FRIEND,
    ITEM_GROUP,
    ITEM_FRIEND_ADD,
};

typedef struct
{
    uint8_t item;
    char namestr[15];
    void *data;
}ITEM;

extern ITEM *sitem;
ITEM *ritem;

void list_start(void);
void list_addfriend(FRIEND *f);
void list_addfriend2(FRIEND *f, FRIENDREQ *req);
void list_addgroup(GROUPCHAT *g);
void list_addfriendreq(FRIENDREQ *f);
void list_deletesitem(void);
void list_deleteritem(void);

void list_selectaddfriend(void);
void list_selectsettings(void);
void list_selectswap(void);

void list_scale(void);

void list_draw(void *n, int x, int y, int width, int height);
void list_freeall(void);

_Bool list_mmove(void *n, int x, int y, int width, int height, int mx, int my, int dx, int dy);
_Bool list_mdown(void *n);
_Bool list_mright(void *n);
_Bool list_mwheel(void *n, int height, double d);
_Bool list_mup(void *n);
_Bool list_mleave(void *n);
