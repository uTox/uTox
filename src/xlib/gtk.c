#include "gtk.h"

#include "../avatar.h"
#include "../chatlog.h"
#include "../debug.h"
#include "../file_transfers.h"
#include "../filesys.h"
#include "../flist.h"
#include "../friend.h"
#include "../macros.h"
#include "../text.h"
#include "../tox.h"
#include "../ui.h"
#include "../utox.h"

#include "../main.h"

#include "../native/thread.h"

#include "stb.h"

#include <dlfcn.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define LIBGTK_FILENAME "libgtk-3.so"
#define LIBGTK_FILENAME_FALLBACK "libgtk-3.so.0"

#define GTK_FILE_CHOOSER_ACTION_OPEN 0
#define GTK_FILE_CHOOSER_ACTION_SAVE 1

#define GTK_RESPONSE_ACCEPT -3
#define GTK_RESPONSE_CANCEL -6

typedef struct GSList GSList;
struct GSList {
    void *  data;
    GSList *next;
};

/* Each of the following transiently segfaults we we use the pre-existing name.
 * So we have to clobber each of the real names with something localized */
void (*utoxGTK_free)(void *);
void (*utoxGTK_slist_free)(GSList *);
unsigned long (*utoxGTK_signal_connect_data)(void *, const char *, void *, void *, void *, int);
void (*utoxGTK_object_unref)(void *);

void (*utoxGTK_init)(int *, char ***);
bool (*utoxGTK_events_pending)(void);
bool (*utoxGTK_main_iteration)(void);
void (*utoxGTK_widget_set_margin_left)(void *, int);
void (*utoxGTK_widget_set_margin_right)(void *, int);
void (*utoxGTK_widget_destroy)(void *);
void *(*utoxGTK_message_dialog_new)(void *, int, int, int, const char *, ...);
int (*utoxGTK_dialog_run)(void *);

void *(*utoxGTK_file_chooser_dialog_new)(const char *, void *, int, const char *, ...);
void (*utoxGTK_file_chooser_set_select_multiple)(void *, bool);
void (*utoxGTK_file_chooser_set_current_name)(void *, const char *);
char *(*utoxGTK_file_chooser_get_filename)(void *);
GSList *(*utoxGTK_file_chooser_get_filenames)(void *);
void (*utoxGTK_file_chooser_set_do_overwrite_confirmation)(void *, bool);
void (*utoxGTK_file_chooser_set_filter)(void *, void *);
char *(*utoxGTK_file_chooser_get_preview_filename)(void *);
void (*utoxGTK_file_chooser_set_preview_widget)(void *, void *);
void (*utoxGTK_file_chooser_set_preview_widget_active)(void *, bool);

void *(*utoxGTK_file_filter_new)(void);
void (*utoxGTK_file_filter_add_mime_type)(void *, const char *);

void *(*utoxGTK_image_new)(void);
void (*utoxGTK_image_set_from_pixbuf)(void *, void *);

void *(*utoxGDK_pixbuf_new_from_file)(const char *, void **);
void *(*utoxGDK_pixbuf_new_from_file_at_size)(const char *, int, int, void **);
int (*utoxGDK_pixbuf_get_width)(const void *);
int (*utoxGDK_pixbuf_get_height)(const void *);

static bool utoxGTK_open;

static void update_image_preview(void *filechooser, void *image) {
#define MAX_PREVIEW_SIZE 256
    char *filename = utoxGTK_file_chooser_get_preview_filename(filechooser);
    if (!filename)
        return;

    // load preview
    void *pixbuf = utoxGDK_pixbuf_new_from_file(filename, NULL);

    if (!pixbuf) {
        utoxGTK_free(filename);
        utoxGTK_file_chooser_set_preview_widget_active(filechooser, false);
        return;
    }

    // if preview too big load smaller
    if (utoxGDK_pixbuf_get_width(pixbuf) > MAX_PREVIEW_SIZE || utoxGDK_pixbuf_get_height(pixbuf) > MAX_PREVIEW_SIZE) {
        utoxGTK_object_unref(pixbuf);
        pixbuf = utoxGDK_pixbuf_new_from_file_at_size(filename, MAX_PREVIEW_SIZE, MAX_PREVIEW_SIZE, NULL);
    }

    utoxGTK_free(filename);

    if (!pixbuf) {
        utoxGTK_file_chooser_set_preview_widget_active(filechooser, false);
        return;
    }

    // pad to MAX_PREVIEW_SIZE + 3px margins
    int margin = (MAX_PREVIEW_SIZE + 6 - utoxGDK_pixbuf_get_width(pixbuf)) / 2;
    utoxGTK_widget_set_margin_left(image, margin);
    utoxGTK_widget_set_margin_right(image, margin);

    // set preview
    utoxGTK_image_set_from_pixbuf(image, pixbuf);
    utoxGTK_object_unref(pixbuf);
    utoxGTK_file_chooser_set_preview_widget_active(filechooser, true);
}

static void ugtk_opensendthread(void *args) {
    size_t fid = (size_t)args;

    void *dialog = utoxGTK_file_chooser_dialog_new((const char *)S(SEND_FILE), NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
                                                   "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
    utoxGTK_file_chooser_set_select_multiple(dialog, true);

    void *preview = utoxGTK_image_new();
    utoxGTK_file_chooser_set_preview_widget(dialog, preview);
    utoxGTK_signal_connect_data(dialog, "update-preview", update_image_preview, preview, NULL, 0);

    int result = utoxGTK_dialog_run(dialog);
    if (result == GTK_RESPONSE_ACCEPT) {
        GSList *list = utoxGTK_file_chooser_get_filenames(dialog), *p = list;
        while (p) {
            UTOX_MSG_FT *send = calloc(1, sizeof(UTOX_MSG_FT));
            if (!send) {
                LOG_ERR("GTK", "GTK:\tUnabled to malloc for to send an FT msg");
                while(p) {
                    utoxGTK_free(p->data);
                    p = p->next;
                }
                utoxGTK_slist_free(list);
                utoxGTK_open = false;
                return;
            }
            LOG_INFO("GTK", "Sending file %s" , p->data);
            send->file = fopen(p->data, "rb");
            send->name = (uint8_t*)strdup(p->data);
            postmessage_toxcore(TOX_FILE_SEND_NEW, (uint32_t)fid, 0, send);
            utoxGTK_free(p->data);
            p = p->next;
        }
        utoxGTK_slist_free(list);
    }

    utoxGTK_widget_destroy(dialog);
    while (utoxGTK_events_pending()) {
        utoxGTK_main_iteration();
    }

    utoxGTK_open = false;
}

static void ugtk_openavatarthread(void *UNUSED(args)) {
    void *dialog =
        utoxGTK_file_chooser_dialog_new((const char *)S(SELECT_AVATAR_TITLE), NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
                                        "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
    void *filter = utoxGTK_file_filter_new();
    utoxGTK_file_filter_add_mime_type(filter, "image/jpeg");
    utoxGTK_file_filter_add_mime_type(filter, "image/png");
    utoxGTK_file_filter_add_mime_type(filter, "image/bmp");
    utoxGTK_file_filter_add_mime_type(filter, "image/gif");
    utoxGTK_file_chooser_set_filter(dialog, filter);

    void *preview = utoxGTK_image_new();
    utoxGTK_file_chooser_set_preview_widget(dialog, preview);
    utoxGTK_signal_connect_data(dialog, "update-preview", update_image_preview, preview, NULL, 0);

    while (utoxGTK_dialog_run(dialog) == GTK_RESPONSE_ACCEPT) {
        char *filename = utoxGTK_file_chooser_get_filename(dialog);
        int   size;

        int      width, height, bpp;
        uint8_t *img       = stbi_load(filename, &width, &height, &bpp, 0);
        uint8_t *file_data = stbi_write_png_to_mem(img, 0, width, height, bpp, &size);
        free(img);

        utoxGTK_free(filename);
        if (!file_data) {
            void *message_dialog = utoxGTK_message_dialog_new(dialog, 0, 1, 2, (const char *)S(CANT_FIND_FILE_OR_EMPTY));
            utoxGTK_dialog_run(message_dialog);
            utoxGTK_widget_destroy(message_dialog);
        } else if (size > UTOX_AVATAR_MAX_DATA_LENGTH) {
            free(file_data);
            char size_str[16];
            int  len          = sprint_humanread_bytes(size_str, sizeof(size_str), UTOX_AVATAR_MAX_DATA_LENGTH);
            char err_str[265] = { 0 };
            snprintf((char *)err_str, 265, "%s%.*s (%ikb loaded)", S(AVATAR_TOO_LARGE_MAX_SIZE_IS), len, size_str,
                     (size / 1024));
            void *message_dialog = utoxGTK_message_dialog_new(dialog, 0, 1, 2, err_str);
            utoxGTK_dialog_run(message_dialog);
            utoxGTK_widget_destroy(message_dialog);
        } else {
            postmessage_utox(SELF_AVATAR_SET, size, 0, file_data);
            break;
        }
    }

    utoxGTK_widget_destroy(dialog);
    while (utoxGTK_events_pending()) {
        utoxGTK_main_iteration();
    }

    utoxGTK_open = false;
}

void show_messagebox(const char *UNUSED(caption), uint16_t UNUSED(caption_length),
                     const char *message, uint16_t UNUSED(message_length)) {
    utoxGTK_open = true;

    void *dialog = utoxGTK_message_dialog_new(NULL, 0, 1, 1, message);
    utoxGTK_dialog_run(dialog);
    utoxGTK_widget_destroy(dialog);

    while (utoxGTK_events_pending()) {
        utoxGTK_main_iteration();
    }

    utoxGTK_open = false;
}

static void ugtk_savethread(void *args) {
    FILE_TRANSFER *file = args;

    while (1) { // TODO, save current dir, and filename and preload them to gtk dialog if save fails.
        /* Create a GTK save window */
        void *dialog =
            utoxGTK_file_chooser_dialog_new((const char *)S(WHERE_TO_SAVE_FILE), NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
                                            "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
        /* Get incoming file name for GTK */
        char buf[file->name_length + 1];
        snprintf(buf, file->name_length + 1, "%.*s", (int)file->name_length, file->name);

        utoxGTK_file_chooser_set_current_name(dialog, buf);
        utoxGTK_file_chooser_set_do_overwrite_confirmation(dialog, true);
        /* Users can create folders when saving. */ // TODO ENABLE BELOW!
        // utoxGTK_file_chooser_set_create_folders(dialog, TRUE);
        int result = utoxGTK_dialog_run(dialog);

        if (result == GTK_RESPONSE_ACCEPT) {
            char *name = utoxGTK_file_chooser_get_filename(dialog);
            char *path = strdup(name);
            // utoxGTK_free(name)

            LOG_TRACE("GTK", "name: %s\npath: %s" , name, path);

            /* can we really write this file? */
            FILE *fp = fopen(path, "w");
            if (fp == NULL) {
                /* No, we can't display error, jump to top. */
                if (errno == EACCES) {
                    LOG_TRACE("GTK", "File write permission denied." );
                    void *errordialog = utoxGTK_message_dialog_new(dialog, 1, 3, 2,
                                                                   // parent, destroy_with_parent,
                                                                   // utoxGTK_error_message, utoxGTK_buttons_close
                                                                   "Error writing to file '%s'", name);
                    utoxGTK_dialog_run(errordialog);
                    utoxGTK_widget_destroy(errordialog);
                    utoxGTK_widget_destroy(dialog);
                    free(path);
                    continue;
                }
                LOG_TRACE("GTK", "Unknown file write error..." );
                free(path);
                break;
            }

            fclose(fp);
            /* write test passed, we're done! */
            utoxGTK_widget_destroy(dialog);
            utoxGTK_main_iteration();
            utoxGTK_widget_destroy(dialog);
            postmessage_utox(FILE_INCOMING_ACCEPT, file->friend_number, (file->file_number >> 16), path);
            break;
        } else if (result == GTK_RESPONSE_CANCEL) {
            LOG_TRACE("GTK", "Aborting in progress file..." );
        }
        /* catch all */
        utoxGTK_widget_destroy(dialog);
        break;
    }

    while (utoxGTK_events_pending()) {
        utoxGTK_main_iteration();
    }

    utoxGTK_open = false;
}

static void ugtk_save_data_thread(void *args) {
    MSG_HEADER *msg = args;
    void *dialog = utoxGTK_file_chooser_dialog_new((const char *)S(SAVE_FILE), NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
                                                   "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);
    utoxGTK_file_chooser_set_current_name(dialog, msg->via.ft.name);
    int result = utoxGTK_dialog_run(dialog);
    if (result == GTK_RESPONSE_ACCEPT) {
        char *name = utoxGTK_file_chooser_get_filename(dialog);

        FILE *fp = fopen(name, "wb");
        if (fp) {
            fwrite(msg->via.ft.data, msg->via.ft.data_size, 1, fp);
            fclose(fp);

            if (!msg->via.ft.path) {
                msg->via.ft.path_length = strlen(name);
                msg->via.ft.path = calloc(1, msg->via.ft.path_length + 1);
            }

            if (msg->via.ft.path) {
                snprintf((char *)msg->via.ft.path, UTOX_FILE_NAME_LENGTH, "%s", name);
            }
            msg->via.ft.inline_png = false;
        }
    }

    utoxGTK_widget_destroy(dialog);
    while (utoxGTK_events_pending()) {
        utoxGTK_main_iteration();
    }

    utoxGTK_open = false;
}

static void ugtk_save_chatlog_thread(void *args) {
    size_t friend_number = (size_t)args;
    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_ERR("GTK", "Could not get friend with number: %u", friend_number);
        utoxGTK_open = false;
        return;
    }

    char name[TOX_MAX_NAME_LENGTH + sizeof ".txt"];
    snprintf(name, sizeof name, "%.*s.txt", (int)f->name_length, f->name);

    void *dialog = utoxGTK_file_chooser_dialog_new((const char *)S(SAVE_FILE), NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
                                                   "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);
    utoxGTK_file_chooser_set_current_name(dialog, name);
    int result = utoxGTK_dialog_run(dialog);
    if (result == GTK_RESPONSE_ACCEPT) {
        char *file_name = utoxGTK_file_chooser_get_filename(dialog);

        FILE *fp = fopen(file_name, "wb");
        if (fp) {
            utox_export_chatlog(f->id_str, fp);
        }
    }

    utoxGTK_widget_destroy(dialog);
    while (utoxGTK_events_pending()) {
        utoxGTK_main_iteration();
    }

    utoxGTK_open = false;
}

static void ugtk_save_image_png_thread(void *args) {
    FILE_IMAGE *image = args;

    char name[TOX_MAX_NAME_LENGTH + sizeof ".png"] = { 0 };
    snprintf(name, sizeof name, "%s.png", image->name);

    void *dialog = utoxGTK_file_chooser_dialog_new((const char *)S(SAVE_FILE), NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
                                                   "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);
    void *filter = utoxGTK_file_filter_new();
    utoxGTK_file_filter_add_mime_type(filter, "image/png");
    utoxGTK_file_chooser_set_filter(dialog, filter);

    utoxGTK_file_chooser_set_current_name(dialog, name);
    int result = utoxGTK_dialog_run(dialog);
    if (result == GTK_RESPONSE_ACCEPT) {
        char *file_name = utoxGTK_file_chooser_get_filename(dialog);

        FILE *file = fopen(file_name, "wb");
        if (file) {
            fwrite(image->data, image->data_size, 1, file);
            fclose(file);
        } else {
            LOG_ERR("GTK", "Could not open file %s for write.", file_name);
        }

        free(file_name);
    }

    utoxGTK_widget_destroy(dialog);

    while (utoxGTK_events_pending()) {
        utoxGTK_main_iteration();
    }

    utoxGTK_open = false;
    free(image);
}

void ugtk_openfilesend(void) {
    if (utoxGTK_open) {
        return;
    }

    FRIEND *f = flist_get_sel_friend();
    if (!f) {
        LOG_ERR("GTK", "Unable to get friend from flist.");
        return;
    }

    utoxGTK_open = true;
    uint32_t number = f->number;
    thread(ugtk_opensendthread, (void*)(size_t)number);
}

void ugtk_openfileavatar(void) {
    if (utoxGTK_open) {
        return;
    }
    utoxGTK_open = true;
    thread(ugtk_openavatarthread, NULL);
}

void ugtk_native_select_dir_ft(uint32_t UNUSED(fid), FILE_TRANSFER *file) {
    if (utoxGTK_open) {
        return;
    }
    utoxGTK_open = true;
    thread(ugtk_savethread, file);
}

void ugtk_file_save_inline(MSG_HEADER *msg) {
    if (utoxGTK_open) {
        return;
    }
    utoxGTK_open = true;
    thread(ugtk_save_data_thread, msg);
}

void ugtk_file_save_image_png(FILE_IMAGE *image) {
    if (utoxGTK_open) {
        return;
    }
    utoxGTK_open = true;
    thread(ugtk_save_image_png_thread, image);
}

void ugtk_save_chatlog(uint32_t friend_number) {
    if (utoxGTK_open) {
        return;
    }

    // We just care about sending a single uint, but we don't want to overflow a buffer
    size_t fnum = friend_number;
    utoxGTK_open = true;
    thread(ugtk_save_chatlog_thread, (void *)fnum); // No need to create and pass a pointer for a single u32
}

/* macro to link and test each of the gtk functions we need.
 * This is likely more specific than in it needs to be, so
 * when you rewrite this, aim for generic */
#define U_DLLOAD(trgt, name)                                           \
    do {                                                               \
        utoxGTK_##name = dlsym(lib, #trgt "_" #name);                  \
        if (!utoxGTK_##name) {                                         \
            LOG_ERR("GTK", "Unable to load " #name " (%s)", dlerror()); \
            dlclose(lib);                                              \
            return NULL;                                               \
        }                                                              \
    } while (0)

#define U_DLLOAD_GDK(name)                                             \
    do {                                                               \
        utoxGDK_##name = dlsym(lib, "gdk_" #name);                     \
        if (!utoxGDK_##name) {                                         \
            LOG_ERR("GTK", "Unable to load " #name " (%s)", dlerror()); \
            dlclose(lib);                                              \
            return NULL;                                               \
        }                                                              \
    } while (0)


void *ugtk_load(void) {
    // return NULL;
    void *lib = dlopen(LIBGTK_FILENAME, RTLD_LAZY);
    if (!lib) { //try again with libgtk-3.so.0 if the first one failed
        LOG_INFO("GTK", "Failed loading: %s. Falling back to %s", LIBGTK_FILENAME, LIBGTK_FILENAME_FALLBACK);
        lib = dlopen(LIBGTK_FILENAME_FALLBACK, RTLD_LAZY);
    }

    if (lib) {
        LOG_TRACE("GTK", "have GTK" );

        U_DLLOAD(gtk, init);
        U_DLLOAD(gtk, main_iteration);
        U_DLLOAD(gtk, events_pending);
        U_DLLOAD(gtk, file_chooser_dialog_new);
        U_DLLOAD(gtk, file_filter_new);
        U_DLLOAD(gtk, message_dialog_new);
        U_DLLOAD(gtk, dialog_run);
        U_DLLOAD(gtk, file_chooser_get_filename);
        U_DLLOAD(gtk, file_chooser_get_filenames);
        U_DLLOAD(gtk, file_chooser_set_do_overwrite_confirmation);
        U_DLLOAD(gtk, file_chooser_set_select_multiple);
        U_DLLOAD(gtk, file_chooser_set_current_name);
        U_DLLOAD(gtk, file_chooser_set_filter);
        U_DLLOAD(gtk, file_filter_add_mime_type);
        U_DLLOAD(gtk, widget_destroy);
        U_DLLOAD(gtk, file_chooser_get_preview_filename);
        U_DLLOAD(gtk, file_chooser_set_preview_widget_active);
        U_DLLOAD(gtk, file_chooser_set_preview_widget);
        U_DLLOAD(gtk, image_new);
        U_DLLOAD(gtk, image_set_from_pixbuf);
        U_DLLOAD(gtk, widget_set_margin_left);
        U_DLLOAD(gtk, widget_set_margin_right);

        U_DLLOAD(g, slist_free);
        U_DLLOAD(g, free);
        U_DLLOAD(g, signal_connect_data);
        U_DLLOAD(g, object_unref);

        U_DLLOAD_GDK(pixbuf_new_from_file);
        U_DLLOAD_GDK(pixbuf_new_from_file_at_size);
        U_DLLOAD_GDK(pixbuf_get_width);
        U_DLLOAD_GDK(pixbuf_get_height);


        utoxGTK_init(NULL, NULL);
        return lib;
    }
    return NULL;
}
