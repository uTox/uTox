#include "../main.h"

#include <stdbool.h>
#include <stdlib.h>
#include <dlfcn.h>

#define LIBGTK_FILENAME "libgtk-3.so.0"

#define GTK_FILE_CHOOSER_ACTION_OPEN 0
#define GTK_FILE_CHOOSER_ACTION_SAVE 1

#define GTK_RESPONSE_ACCEPT -3
#define GTK_RESPONSE_CANCEL -6

typedef struct GSList GSList;
struct GSList {
    void *data;
    GSList *next;
};

void (*gtk_init)(int*, char***);
bool (*gtk_events_pending)(void);
bool (*gtk_main_iteration)(void);

void (*gtk_widget_set_margin_left)(void*, int);
void (*gtk_widget_set_margin_right)(void*, int);
void (*gtk_widget_destroy)(void*);

unsigned long (*g_signal_connect_data)(void*, const char*, void*, void*, void*, int);
void (*g_object_unref)(void*);
void (*g_slist_free_utox)(GSList*);
void (*g_free_utox)(void*); // this can't be called g_free because it causes segfaults on some machines if it is

void* (*gtk_message_dialog_new)(void*, int, int, int, const char*, ...);

int (*gtk_dialog_run)(void*);

void* (*gtk_file_chooser_dialog_new)(const char*, void*, int, const char*, ...);
void (*gtk_file_chooser_set_select_multiple)(void*, bool);
void (*gtk_file_chooser_set_current_name)(void*, const char*);
char* (*gtk_file_chooser_get_filename)(void*);
GSList* (*gtk_file_chooser_get_filenames)(void*);
void (*gtk_file_chooser_set_do_overwrite_confirmation)(void*, bool);
void (*gtk_file_chooser_set_filter)(void*, void*);
char* (*gtk_file_chooser_get_preview_filename)(void*);
void (*gtk_file_chooser_set_preview_widget)(void*, void*);
void (*gtk_file_chooser_set_preview_widget_active)(void*, bool);

void* (*gtk_file_filter_new)(void);
void (*gtk_file_filter_add_mime_type)(void*, const char*);

void* (*gtk_image_new)(void);
void (*gtk_image_set_from_pixbuf)(void*, void*);

void* (*gdk_pixbuf_new_from_file)(const char*, void**);
void* (*gdk_pixbuf_new_from_file_at_size)(const char*, int, int, void**);
int (*gdk_pixbuf_get_width)(const void*);
int (*gdk_pixbuf_get_height)(const void*);

volatile bool gtk_open;

static void update_image_preview(void *filechooser, void *image) {
#define MAX_PREVIEW_SIZE 256
    char *filename = gtk_file_chooser_get_preview_filename(filechooser);
    if (!filename)
        return;

    // load preview
    void *pixbuf = gdk_pixbuf_new_from_file(filename, NULL);

    if (!pixbuf) {
        g_free_utox(filename);
        gtk_file_chooser_set_preview_widget_active(filechooser, false);
        return;
    }

    // if preview too big load smaller
    if (gdk_pixbuf_get_width(pixbuf) > MAX_PREVIEW_SIZE || gdk_pixbuf_get_height(pixbuf) > MAX_PREVIEW_SIZE) {
        g_object_unref(pixbuf);
        pixbuf = gdk_pixbuf_new_from_file_at_size(filename, MAX_PREVIEW_SIZE, MAX_PREVIEW_SIZE, NULL);
    }

    g_free_utox(filename);

    if (!pixbuf) {
        gtk_file_chooser_set_preview_widget_active(filechooser, false);
        return;
    }

    // pad to MAX_PREVIEW_SIZE + 3px margins
    int margin = (MAX_PREVIEW_SIZE + 6 - gdk_pixbuf_get_width(pixbuf)) / 2;
    gtk_widget_set_margin_left(image, margin);
    gtk_widget_set_margin_right(image, margin);

    // set preview
    gtk_image_set_from_pixbuf(image, pixbuf);
    g_object_unref(pixbuf);
    gtk_file_chooser_set_preview_widget_active(filechooser, true);
}

static void gtk_opensendthread(void *args) {
    uint16_t fid = (size_t)args;

    void *dialog = gtk_file_chooser_dialog_new((const char *)S(SEND_FILE), NULL,
            GTK_FILE_CHOOSER_ACTION_OPEN,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Open", GTK_RESPONSE_ACCEPT,
            NULL);
    gtk_file_chooser_set_select_multiple(dialog, true);

    void *preview = gtk_image_new();
    gtk_file_chooser_set_preview_widget(dialog, preview);
    g_signal_connect_data(dialog, "update-preview", update_image_preview, preview, NULL, 0);

    int result = gtk_dialog_run(dialog);
    if(result == GTK_RESPONSE_ACCEPT) {
        char *out = malloc(65536), *outp = out;
        GSList *list = gtk_file_chooser_get_filenames(dialog), *p = list;
        while(p) {
            outp = stpcpy(outp, p->data);
            *outp++ = '\n';
            g_free_utox(p->data);
            p = p->next;
        }
        *outp = 0;
        g_slist_free_utox(list);
        debug("files: %s\n", out);

        //dont call this from this thread
        postmessage_toxcore(TOX_FILE_SEND_NEW, fid, 0xFFFF, out);
    }

    gtk_widget_destroy(dialog);
    while(gtk_events_pending()) {
        gtk_main_iteration();
    }

    gtk_open = false;
}

static void gtk_openavatarthread(void *UNUSED(args)) {
    void *dialog = gtk_file_chooser_dialog_new((const char *)S(SELECT_AVATAR_TITLE), NULL,
            GTK_FILE_CHOOSER_ACTION_OPEN,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Open", GTK_RESPONSE_ACCEPT,
            NULL);
    void *filter = gtk_file_filter_new();
    gtk_file_filter_add_mime_type(filter, "image/jpeg");
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_filter_add_mime_type(filter, "image/bmp");
    gtk_file_filter_add_mime_type(filter, "image/gif");
    gtk_file_chooser_set_filter(dialog, filter);

    void *preview = gtk_image_new();
    gtk_file_chooser_set_preview_widget(dialog, preview);
    g_signal_connect_data(dialog, "update-preview", update_image_preview, preview, NULL, 0);

    while (gtk_dialog_run(dialog) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(dialog);
        int size;

        int width, height, bpp;
        uint8_t *img = stbi_load(filename, &width, &height, &bpp, 0);
        uint8_t *file_data = stbi_write_png_to_mem(img, 0, width, height, bpp, &size);
        free(img);

        g_free_utox(filename);
        if (!file_data) {
            void *message_dialog = gtk_message_dialog_new(dialog, 0, 1, 2, (const char *)S(CANT_FIND_FILE_OR_EMPTY));
            gtk_dialog_run(message_dialog);
            gtk_widget_destroy(message_dialog);
        } else if (size > UTOX_AVATAR_MAX_DATA_LENGTH) {
            free(file_data);
            char_t size_str[16];
            int len = sprint_humanread_bytes(size_str, sizeof(size_str), UTOX_AVATAR_MAX_DATA_LENGTH);
            void *message_dialog = gtk_message_dialog_new(dialog, 0, 1, 2, "%s%.*s.", S(AVATAR_TOO_LARGE_MAX_SIZE_IS), len, size_str);
            gtk_dialog_run(message_dialog);
            gtk_widget_destroy(message_dialog);
        } else {
            postmessage(SELF_AVATAR_SET, size, 0, file_data);
            break;
        }
    }

    gtk_widget_destroy(dialog);
    while(gtk_events_pending()) {
        gtk_main_iteration();
    }

    gtk_open = false;
}

static void gtk_savethread(void *args) {
    MSG_FILE *file = args;

    //WHY?!
    uint16_t fid = file->progress;
    file->progress = 0;
    //WHY?!

    while(1){ //TODO, save current dir, and filename and preload them to gtk dialog if save fails.
        /* Create a GTK save window */
        void *dialog = gtk_file_chooser_dialog_new((const char *)S(SELECT_AVATAR_TITLE), NULL,
                GTK_FILE_CHOOSER_ACTION_SAVE,
                "_Cancel", GTK_RESPONSE_CANCEL,
                "_Open", GTK_RESPONSE_ACCEPT,
                NULL);
        /* Get incoming file name*/
        char buf[sizeof(file->name) + 1];
        memcpy(buf, file->name, file->name_length);
        buf[file->name_length] = 0;
        /* give gtk the file name our friend is sending. */
        gtk_file_chooser_set_current_name(dialog, buf);
        /* Prompt to overwrite */
        gtk_file_chooser_set_do_overwrite_confirmation(dialog, true);
        /* Users can create folders when saving. */ //TODO ENABLE BELOW!
        //gtk_file_chooser_set_create_folders(dialog, TRUE);
        int result = gtk_dialog_run(dialog);
        /* If user is ready to save check then pass to utox. */
        if(result == GTK_RESPONSE_ACCEPT) {
            char *name = gtk_file_chooser_get_filename(dialog);
            char *path = strdup(name);
            //g_free_utox(name)

            debug("name: %s\npath: %s\n", name, path);

            /* can we really write this file? */
            FILE *fp = fopen(path, "w");
            if(fp == NULL){
                /* No, we can't display error, jump to top. */
                if(errno == EACCES){
                    debug("File write permission denied.\n");
                    void *errordialog = gtk_message_dialog_new(dialog, 1, 3, 2,
                            //parent, destroy_with_parent, gtk_error_message, gtk_buttons_close
                                            "Error writing to file '%s'", name);
                    gtk_dialog_run(errordialog);
                    gtk_widget_destroy(errordialog);
                    gtk_widget_destroy(dialog);
                    continue;
                } else {
                    debug("Unknown file write error...\n");
                }
            } else {
                fclose(fp);
                /* write test passed, we're done! */
                gtk_widget_destroy(dialog);
                gtk_main_iteration();
                gtk_widget_destroy(dialog);
                postmessage(FILE_INCOMING_ACCEPT, fid, (file->filenumber >> 16), path);
                break;
            }
        } else if (result == GTK_RESPONSE_CANCEL) {
            debug("Aborting in progress file...\n");
        }
        /* catch all */
        gtk_widget_destroy(dialog);
        break;
    }

    while(gtk_events_pending()) {
        gtk_main_iteration();
    }

    gtk_open = false;
}

static void gtk_savedatathread(void *args) {
    MSG_FILE *file = args;
    void *dialog = gtk_file_chooser_dialog_new((const char *)S(SAVE_FILE), NULL,
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Save", GTK_RESPONSE_ACCEPT,
            NULL);
    gtk_file_chooser_set_current_name(dialog, "inline.png");
    int result = gtk_dialog_run(dialog);
    if(result == GTK_RESPONSE_ACCEPT) {
        char *name = gtk_file_chooser_get_filename(dialog);

        FILE *fp = fopen(name, "wb");
        if(fp) {
            fwrite(file->path, file->size, 1, fp);
            fclose(fp);

            free(file->path);
            file->path = (uint8_t*)strdup(name);
            file->inline_png = 0;
        }
    }

    gtk_widget_destroy(dialog);
    while(gtk_events_pending()) {
        gtk_main_iteration();
    }

    gtk_open = false;
}

void gtk_openfilesend(void) {
    if(gtk_open) {
        return;
    }
    gtk_open = true;
    thread(gtk_opensendthread, (void*)(size_t)((FRIEND*)selected_item->data - friend));
}

void gtk_openfileavatar(void) {
    if(gtk_open) {
        return;
    }
    gtk_open = true;
    thread(gtk_openavatarthread, NULL);
}

void gtk_savefilerecv(uint32_t fid, MSG_FILE *file) {
    if(gtk_open) {
        return;
    }
    gtk_open = true;
    file->progress = fid;
    thread(gtk_savethread, file);
}

void gtk_savefiledata(MSG_FILE *file) {
    if(gtk_open) {
        return;
    }
    gtk_open = true;
    thread(gtk_savedatathread, file);
}

void* gtk_load(void) {
    void *lib = dlopen(LIBGTK_FILENAME, RTLD_LAZY);
    if(lib) {
        debug("have GTK\n");

        gtk_init = dlsym(lib, "gtk_init");
        gtk_main_iteration = dlsym(lib, "gtk_main_iteration");
        gtk_events_pending = dlsym(lib, "gtk_events_pending");
        gtk_file_chooser_dialog_new = dlsym(lib, "gtk_file_chooser_dialog_new");
        gtk_file_filter_new = dlsym(lib, "gtk_file_filter_new");
        gtk_message_dialog_new = dlsym(lib, "gtk_message_dialog_new");
        gtk_dialog_run = dlsym(lib, "gtk_dialog_run");
        gtk_file_chooser_get_filename = dlsym(lib, "gtk_file_chooser_get_filename");
        gtk_file_chooser_get_filenames = dlsym(lib, "gtk_file_chooser_get_filenames");
        gtk_file_chooser_set_do_overwrite_confirmation = dlsym(lib, "gtk_file_chooser_set_do_overwrite_confirmation");
        gtk_file_chooser_set_select_multiple = dlsym(lib, "gtk_file_chooser_set_select_multiple");
        gtk_file_chooser_set_current_name = dlsym(lib, "gtk_file_chooser_set_current_name");
        gtk_file_chooser_set_filter = dlsym(lib, "gtk_file_chooser_set_filter");
        gtk_file_filter_add_mime_type = dlsym(lib, "gtk_file_filter_add_mime_type");
        gtk_widget_destroy = dlsym(lib, "gtk_widget_destroy");
        g_slist_free_utox = dlsym(lib, "g_slist_free");
        g_free_utox = dlsym(lib, "g_free");
        g_signal_connect_data = dlsym(lib, "g_signal_connect_data");
        g_object_unref = dlsym(lib, "g_object_unref");
        gtk_file_chooser_get_preview_filename = dlsym(lib, "gtk_file_chooser_get_preview_filename");
        gtk_file_chooser_set_preview_widget_active = dlsym(lib, "gtk_file_chooser_set_preview_widget_active");
        gtk_file_chooser_set_preview_widget = dlsym(lib, "gtk_file_chooser_set_preview_widget");
        gtk_image_new = dlsym(lib, "gtk_image_new");
        gtk_image_set_from_pixbuf = dlsym(lib, "gtk_image_set_from_pixbuf");
        gdk_pixbuf_new_from_file = dlsym(lib, "gdk_pixbuf_new_from_file");
        gdk_pixbuf_new_from_file_at_size = dlsym(lib, "gdk_pixbuf_new_from_file_at_size");
        gdk_pixbuf_get_width = dlsym(lib, "gdk_pixbuf_get_width");
        gdk_pixbuf_get_height = dlsym(lib, "gdk_pixbuf_get_height");
        gtk_widget_set_margin_left = dlsym(lib, "gtk_widget_set_margin_left");
        gtk_widget_set_margin_right = dlsym(lib, "gtk_widget_set_margin_right");

        if(!gtk_init || !gtk_main_iteration || !gtk_events_pending || !gtk_file_chooser_dialog_new || !gtk_file_filter_new ||
           !gtk_message_dialog_new || !gtk_dialog_run || !gtk_file_chooser_get_filename || !gtk_file_chooser_get_filenames ||
           !gtk_file_chooser_set_do_overwrite_confirmation || !gtk_file_chooser_set_select_multiple || !gtk_file_chooser_set_current_name ||
           !gtk_file_chooser_set_filter || !gtk_file_filter_add_mime_type || !gtk_widget_destroy || !g_slist_free_utox || !g_free_utox ||
           !gtk_file_chooser_get_preview_filename || !gtk_file_chooser_set_preview_widget_active || !gtk_file_chooser_set_preview_widget ||
           !gtk_image_new || !gtk_image_set_from_pixbuf || !gdk_pixbuf_new_from_file || !gdk_pixbuf_new_from_file_at_size || !gdk_pixbuf_get_width ||
           !gdk_pixbuf_get_height || !gtk_widget_set_margin_left || !gtk_widget_set_margin_right) {
            debug("bad GTK\n");
            dlclose(lib);
        } else {
            gtk_init(NULL, NULL);
            return lib;
        }
    }
    return NULL;
}
