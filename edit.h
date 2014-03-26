
typedef struct
{
    _Bool multiline, mouseover, mousedown;
    uint16_t length, maxlength;
    int x, y, bottom, right;
    uint8_t *data;
    void (*onenter)(void);
}EDIT;

void edit_mousemove(EDIT *edit, int x, int y);
void edit_mousedown(EDIT *edit, int x, int y);
void edit_mouseup(EDIT *edit);
void edit_mouseleave(EDIT *edit);
void edit_rightclick(EDIT *edit, int x, int y);

void edit_draw(EDIT *edit);

void edit_char(uint32_t ch);

void edit_cut(void);
void edit_copy(void);
void edit_paste(void);
void edit_delete(void);
void edit_selectall(void);
void edit_clear(void);

void edit_setfocus(EDIT *edit, _Bool drawold);

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
