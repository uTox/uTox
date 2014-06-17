
#define BM_SCROLLHALF_WIDTH SCROLL_WIDTH
#define BM_SCROLLHALF_HEIGHT (SCROLL_WIDTH / 2)

#define BM_STATUSAREA_WIDTH (10 * SCALE)
#define BM_STATUSAREA_HEIGHT (20 * SCALE)

#define BM_ADD_WIDTH (9 * SCALE)
#define BM_STATUS_WIDTH (5 * SCALE)
#define BM_NMSG_WIDTH (9 * SCALE)

#define BM_LBUTTON_WIDTH (26 * SCALE)
#define BM_LBUTTON_HEIGHT (20 * SCALE)

#define BM_SBUTTON_WIDTH (26 * SCALE)
#define BM_SBUTTON_HEIGHT (10 * SCALE)

#define BM_FT_WIDTH (125 * SCALE)
#define BM_FT_HEIGHT (26 * SCALE)

#define BM_FTM_WIDTH (113 * SCALE)

#define BM_FTB_WIDTH (11 * SCALE)
#define BM_FTB_HEIGHT (12 * SCALE)

#define BM_CONTACT_WIDTH 40//(20 * SCALE)

#define BM_LBICON_WIDTH (11 * SCALE)
#define BM_LBICON_HEIGHT (10 * SCALE)

//fix names
/* rgb */

/* alpha */
void *bm_scroll_bits, *bm_statusarea;

void *bm_status_bits, *bm_nmsg;
void *bm_lbutton, *bm_sbutton;
void *bm_ft, *bm_ftm, *bm_ftb;

void *bm_contact, *bm_group;

void *bm_add, *bm_groups, *bm_transfer, *bm_settings;
void *bm_call, *bm_file;

void svg_draw(void);
