
typedef struct
{
    _Bool multiline, mouseover, mousedown, locked;
    uint16_t length, maxlength;
    int x, y, bottom, right;
    uint8_t *data;
    void (*onenter)(void);
}EDIT;

EDIT *sedit;

void edit_draw(EDIT *edit);
void edit_paste(EDIT *edit, uint8_t *data, uint16_t length);

void edit_name_onenter(void);
void edit_status_onenter(void);
void edit_msg_onenter(void);

#define edit_func(func, ...) \
    if(sitem){switch(sitem->item){ \
        case ITEM_SELF:{ func(&edit_name, ##__VA_ARGS__); func(&edit_status, ##__VA_ARGS__); break; } \
        case ITEM_ADDFRIEND:{func(&edit_addid, ##__VA_ARGS__); func(&edit_addmsg, ##__VA_ARGS__); break;} \
        case ITEM_FRIEND:{func(&edit_msg, ##__VA_ARGS__); break;} \
        case ITEM_GROUP:{func(&edit_msg, ##__VA_ARGS__); break;} \
        }}
