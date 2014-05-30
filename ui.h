
extern EDIT edit_name, edit_status, edit_addid, edit_addmsg, edit_msg;
extern BUTTON button_copyid, button_addfriend, button_newgroup, button_call, button_acceptfriend;
extern SCROLLABLE scroll_list, scroll_self, scroll_add;

extern uint8_t bm_contact_bits[], bm_group_bits[];
extern uint8_t bm_minimize_bits[], bm_maximize_bits[], bm_restore_bits[], bm_exit_bits[], bm_plus_bits[];
extern uint32_t bm_online_bits[], bm_away_bits[], bm_busy_bits[], bm_offline_bits[];

#define edit_func(func, ...) \
    if(sitem){switch(sitem->item){ \
        case ITEM_SELF:{ func(&edit_name, ##__VA_ARGS__); func(&edit_status, ##__VA_ARGS__); break; } \
        case ITEM_ADDFRIEND:{func(&edit_addid, ##__VA_ARGS__); func(&edit_addmsg, ##__VA_ARGS__); break;} \
        case ITEM_FRIEND:{func(&edit_msg, ##__VA_ARGS__); break;} \
        case ITEM_GROUP:{func(&edit_msg, ##__VA_ARGS__); break;} \
        }}

#define button_func(func, ...) \
    if(sitem){switch(sitem->item){ \
        case ITEM_SELF:{func(&button_copyid, ##__VA_ARGS__); break;} \
        case ITEM_ADDFRIEND:{func(&button_addfriend, ##__VA_ARGS__); func(&button_newgroup, ##__VA_ARGS__); break;} \
        case ITEM_FRIEND:{func(&button_call, ##__VA_ARGS__); break;} \
        case ITEM_FRIEND_ADD:{func(&button_acceptfriend, ##__VA_ARGS__); break;} \
        }}

#define scroll_func(func, ...) \
    func(&scroll_list, ##__VA_ARGS__); \
    if(sitem){switch(sitem->item){ \
        case ITEM_SELF:{func(&scroll_self, ##__VA_ARGS__); break;} \
        case ITEM_ADDFRIEND:{func(&scroll_add, ##__VA_ARGS__); break;} \
        }}

void ui_drawmain(void);
void ui_drawbackground(void);
void ui_updatesize(void);

