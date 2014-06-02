
enum
{
    ITEM_NONE,
    ITEM_SELF,
    ITEM_ADDFRIEND,
    ITEM_FRIEND,
    ITEM_GROUP,
    ITEM_FRIEND_ADD
};

typedef struct
{
    uint8_t item;
    char namestr[15];
    void *data;
}ITEM;

ITEM *sitem;

void list_start(void);
void list_addfriend(FRIEND *f);
void list_addfriend2(FRIEND *f, FRIENDREQ *req);
void list_addgroup(GROUPCHAT *g);
void list_addfriendreq(FRIENDREQ *f);
void list_deletesitem(void);
void list_deleteritem(void);

void list_draw(void *n, int x, int y, int width, int height);

_Bool list_mmove(void *n, int x, int y, int dy, int width, int height);
_Bool list_mdown(void *n);
_Bool list_mright(void *n);
_Bool list_mwheel(void *n, int height, double d);
_Bool list_mup(void *n);
_Bool list_mleave(void *n);
