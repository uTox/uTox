#ifndef NATIVE_MAIN_H
#define NATIVE_MAIN_H

#if defined __WIN32__
#include "windows/main.h"
#elif defined __ANDROID__
#include "android/main.h"
#elif defined __OBJC__
#include "cocoa/main.h"
#else
#include "xlib/main.h"
#endif

typedef struct file_transfer FILE_TRANSFER;
typedef struct native_image NATIVE_IMAGE;
typedef struct utox_save UTOX_SAVE;
typedef uint8_t *UTOX_IMAGE;

enum {
    FILTER_NEAREST, // ugly and quick filtering
    FILTER_BILINEAR // prettier and a bit slower filtering
};

// TODO: Sort what isn't sorted.

/* OS-specific cleanup function for when edits are defocused. Commit IME state, etc. */
void edit_will_deactivate(void);

void showkeyboard(bool show);

void copy(int value);
void paste(void);

void openurl(char *str);

void setselection(char *data, uint16_t length);

void update_tray(void);

void redraw(void);
void force_redraw(void); // TODO: as parameter for redraw()?

// inserts/deletes a value into the registry to launch uTox after boot
void launch_at_startup(int is_launch_at_startup);

void desktopgrab(bool video);
void notify(char *title, uint16_t title_length, const char *msg, uint16_t msg_length, void *object, bool is_group);
void setscale(void);
void setscale_fonts(void);

void config_osdefaults(UTOX_SAVE *r);

/* set filtering method used when resizing given image to one of above enum */
void image_set_filter(NATIVE_IMAGE *image, uint8_t filter);

/* set scale of image so that when it's drawn it will be `scale' times as large(2.0 for double size, 0.5 for half, etc.)
 *  notes: theoretically lowest possible scale is (1.0/65536.0), highest is 65536.0, values outside of this range will
 * create weird issues
 *         scaling will be rounded to pixels, so it might not be exact
 */
void image_set_scale(NATIVE_IMAGE *image, double scale);

/* draws an utox image with or without alpha channel into the rect of (x,y,width,height) on the screen,
 * starting at position (imgx,imgy) of the image
 * WARNING: Windows can fail to show the image at all if the rect (imgx,imgy,width,height) contains even 1 pixel outside
 * of
 * the image's size AFTER SCALING, so be careful.
 * TODO: improve this so this function is safer to use */
void draw_image(const NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t imgx, uint32_t imgy);

/* Native wrapper to ready and call draw_image */
void draw_inline_image(uint8_t *img_data, size_t size, uint16_t w, uint16_t h, int x, int y);

/* converts a png to a NATIVE_IMAGE, returns a pointer to it, keeping alpha channel only if keep_alpha is 1 */
NATIVE_IMAGE *utox_image_to_native(const UTOX_IMAGE, size_t size, uint16_t *w, uint16_t *h, bool keep_alpha);

/* free an image created by utox_image_to_native */
void image_free(NATIVE_IMAGE *image);

void openfilesend(void);

uint64_t get_time(void);

/* use the file chooser to pick an avatar and set it as the user's */
void openfileavatar(void);

void native_export_chatlog_init(uint32_t friend_number);

void native_select_dir_ft(uint32_t fid, uint32_t num, FILE_TRANSFER *file);

bool native_move_file(const uint8_t *current_name, const uint8_t *new_name);

// Push-to-talk
/** returns 0 if push to talk is enabled, and the button is up, else returns 1. */
void init_ptt(void);
bool get_ptt_key(void); // Never used. Remove?
bool set_ptt_key(void); // Never used. Remove?
// Returns a bool indicating whether you should send audio or not.
bool check_ptt_key(void);
void exit_ptt(void);

// Threading
void thread(void func(void *), void *args);
void yieldcpu(uint32_t ms);

// Video
void video_frame(uint32_t id, uint8_t *img_data, uint16_t width, uint16_t height, bool resize);
void video_begin(uint32_t id, char *name, uint16_t name_length, uint16_t width, uint16_t height);
void video_end(uint32_t id);

uint16_t native_video_detect(void);
bool native_video_init(void *handle);
void native_video_close(void *handle);
int native_video_getframe(uint8_t *y, uint8_t *u, uint8_t *v, uint16_t width, uint16_t height);
bool  native_video_startread(void);
bool  native_video_endread(void);

// Audio
void audio_detect(void);
bool audio_init(void *handle);
bool audio_close(void *handle);
bool audio_frame(int16_t *buffer);

// OS interface replacements
void flush_file(FILE *file);
int ch_mod(uint8_t *file);
int file_lock(FILE *file, uint64_t start, size_t length);
int file_unlock(FILE *file, uint64_t start, size_t length);

#endif
