
enum
{
    ITEM_NONE,
    ITEM_SELF,
    ITEM_FRIEND,
    ITEM_GROUP,
    ITEM_ADDFRIEND,
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

void list_draw(void);

void list_mousemove(int x, int y, int dy);
void list_mousedown(void);
void list_mouseup(void);
void list_mouseleave(void);
