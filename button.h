
typedef struct
{
    _Bool mouseover, mousedown;
    uint16_t text_length;
    int x, y, width, height;
    uint8_t *text;
    void (*onpress)(void);
}BUTTON;

void button_copyid_onpress(void);
void button_addfriend_onpress(void);
void button_newgroup_onpress(void);


void button_draw(BUTTON *b);
void button_mousemove(BUTTON *b, int x, int y);
void button_mousedown(BUTTON *b, int x, int y);
void button_mouseup(BUTTON *b);
void button_mouseleave(BUTTON *b);


#define button_func(func, ...) \
    if(sitem){switch(sitem->item){ \
        case ITEM_SELF:{func(&button_copyid, ##__VA_ARGS__); break;} \
        case ITEM_ADDFRIEND:{func(&button_addfriend, ##__VA_ARGS__); break;} \
        case ITEM_NEWGROUP:{func(&button_newgroup, ##__VA_ARGS__); break;}\
        }}
