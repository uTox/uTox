
enum
{
    ITEM_NONE,
    ITEM_FRIEND,
    ITEM_GROUP,
    ITEM_SELF,
    ITEM_ADDFRIEND,
    ITEM_FRIENDREQUESTS,
    ITEM_NEWGROUP
};

typedef struct
{
    uint8_t item;
    char namestr[15];
    void *data;
}ITEM;

ITEM *sitem;

static void list_init(void);
void list_addfriend(FRIEND *f);
void list_draw(void);

void list_deletesitem(void);

void list_mousemove(int x, int y, int dy);
void list_mousedown(void);
void list_mouseup(void);
void list_mouseleave(void);

void list_mousewheel(int x, int y, double d);
