#ifdef __APPLE__
#define LIBGTK_FILENAME "libgtk-x11-2.0.dylib"
#else
#define LIBGTK_FILENAME "libgtk-x11-2.0.so.0"
#endif

typedef struct
{
    void *data, *next;
}g_list;

void (*gtk_init)(int*, char***);
_Bool (*gtk_events_pending)(void);
void (*gtk_main_iteration)(void);
void* (*gtk_file_chooser_dialog_new)(const char*, void*, int, const char*, ...);
void* (*gtk_file_filter_new)(void);
void* (*gtk_message_dialog_new)(void*, int, int, int, const char*, ...);
void (*gtk_file_chooser_set_select_multiple)(void*, _Bool);
void (*gtk_file_chooser_set_current_name)(void*, char*);
int (*gtk_dialog_run)(void*);
void* (*gtk_file_chooser_get_filename)(void*);
void* (*gtk_file_chooser_get_filenames)(void*);
void* (*gtk_file_chooser_set_do_overwrite_confirmation)(const char*, void*);
void (*gtk_file_chooser_set_filter)(void*, void*);
void (*gtk_file_filter_add_mime_type)(void*, const char*);
void (*gtk_widget_destroy)(void*);
void (*g_free_utox)(void*); // this can't be called g_free because it causes segvaults on some machines if it is

volatile _Bool gtk_open;

static void gtk_opensendthread(void *args)
{
    uint16_t fid = (size_t)args;

    void *dialog = gtk_file_chooser_dialog_new("Open File", NULL, 0, "gtk-cancel", -6, "gtk-open", -3, NULL);
    gtk_file_chooser_set_select_multiple(dialog, 1);
    int result = gtk_dialog_run(dialog);
    if(result == -3) {
        char *out = malloc(65536), *outp = out;
        g_list *list = gtk_file_chooser_get_filenames(dialog), *p = list;
        while(p) {
            outp = stpcpy(outp, p->data);
            *outp++ = '\n';
            //g_free(p->data)
            p = p->next;
        }
        *outp = 0;
        //g_slist_free(list)
        debug("files: %s\n", out);

        //dont call this from this thread
        postmessage(SEND_FILES, fid, 0xFFFF, out);
    }

    gtk_widget_destroy(dialog);
    while(gtk_events_pending()) {
        gtk_main_iteration();
    }

    gtk_open = 0;
}

static void gtk_openavatarthread(void *UNUSED(args))
{
    void *dialog = gtk_file_chooser_dialog_new("Open Avatar", NULL, 0, "gtk-cancel", -6, "gtk-open", -3, NULL);
    void *filter = gtk_file_filter_new();
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_chooser_set_filter(dialog, filter);

    while (gtk_dialog_run(dialog) == -3) { // -3 means user selected an image
        char *filename = gtk_file_chooser_get_filename(dialog);
        uint32_t size;

        void *file_data = file_raw(filename, &size);
        g_free_utox(filename);
        if (!file_data) {
            void *message_dialog = gtk_message_dialog_new(dialog, 0, 1, 2, (const char *)S(CANT_FIND_FILE_OR_EMPTY));
            gtk_dialog_run(message_dialog);
            gtk_widget_destroy(message_dialog);
        } else if (size > UTOX_AVATAR_MAX_DATA_LENGTH) {
            free(file_data);
            char_t size_str[16];
            int len = sprint_bytes(size_str, sizeof(size_str), UTOX_AVATAR_MAX_DATA_LENGTH);
            void *message_dialog = gtk_message_dialog_new(dialog, 0, 1, 2, "%s%.*s.", S(AVATAR_TOO_LARGE_MAX_SIZE_IS), len, size_str);
            gtk_dialog_run(message_dialog);
            gtk_widget_destroy(message_dialog);
        } else {
            postmessage(SET_AVATAR, size, 0, file_data);
            break;
        }
    }

    gtk_widget_destroy(dialog);
    while(gtk_events_pending()) {
        gtk_main_iteration();
    }

    gtk_open = 0;
}

static void gtk_savethread(void *args){
    MSG_FILE *file = args;
    uint16_t fid = file->progress;
    file->progress = 0;

    // We're going to save this file somewhere, so we should start the transfer to save time.
    postmessage(FILE_START_TEMP, fid, (file->filenumber >> 16), file);
    debug("Saving file to temp dir...\n");

    while(1){ //TODO, save current dir, and filename and preload them to gtk dialog if save fails.
        /* Create a GTK save window */
        void *dialog = gtk_file_chooser_dialog_new("Save File", NULL, 1, "gtk-cancel", -6, "gtk-save", -3, NULL);
        /* Get incoming file name*/
        char buf[sizeof(file->name) + 1];
        memcpy(buf, file->name, file->name_length);
        buf[file->name_length] = 0;
        /* give gtk the file name our friend is sending. */
        gtk_file_chooser_set_current_name(dialog, buf);
        /* Prompt to overwrite */
        gtk_file_chooser_set_do_overwrite_confirmation(dialog, (void*)1);
        /* Users can create folders when saving. */ //TODO ENABLE BELOW!
        //gtk_file_chooser_set_create_folders(dialog, TRUE);
        int result = gtk_dialog_run(dialog);
        /* If user is ready to save check then pass to utox. */
        if(result == -3) { // -3 == GTK_RESPONSE_ACCEPT
            char *name = gtk_file_chooser_get_filename(dialog);
            char *path = strdup(name);
            //g_free(name)

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
                /* write test passed, we're done! */
                gtk_widget_destroy(dialog);
                postmessage(SAVE_FILE, fid, (file->filenumber >> 16), path);
                break;
            }
        } else if (-6) { // -6 == GTK_RESPONSE_CANCEL
            debug("Aborting in progress file...\n");
            postmessage(FILE_ABORT_TEMP, fid, (file->filenumber >> 16), file);
        }
        /* catch all */
        gtk_widget_destroy(dialog);
        break;
    }

    while(gtk_events_pending()) {
        gtk_main_iteration();
    }

    gtk_open = 0;
}

static void gtk_savedatathread(void *args)
{
    MSG_FILE *file = args;
    void *dialog = gtk_file_chooser_dialog_new("Save File", NULL, 1, "gtk-cancel", -6, "gtk-save", -3, NULL);
    gtk_file_chooser_set_current_name(dialog, "inline.png");
    int result = gtk_dialog_run(dialog);
    if(result == -3) {
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

    gtk_open = 0;
}

void gtk_openfilesend(void)
{
    if(gtk_open) {
        return;
    }
    gtk_open = 1;
    thread(gtk_opensendthread, (void*)(size_t)((FRIEND*)sitem->data - friend));
}

void gtk_openfileavatar(void)
{
    if(gtk_open) {
        return;
    }
    gtk_open = 1;
    thread(gtk_openavatarthread, NULL);
}

void gtk_savefilerecv(uint32_t fid, MSG_FILE *file)
{
    if(gtk_open) {
        return;
    }
    gtk_open = 1;
    file->progress = fid;
    thread(gtk_savethread, file);
}

void gtk_savefiledata(MSG_FILE *file)
{
    if(gtk_open) {
        return;
    }
    gtk_open = 1;
    thread(gtk_savedatathread, file);
}

void* gtk_load(void)
{
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
        g_free_utox = dlsym(lib, "g_free");

        if(!gtk_init || !gtk_main_iteration || !gtk_events_pending || !gtk_file_chooser_dialog_new || !gtk_file_filter_new ||
           !gtk_message_dialog_new || !gtk_dialog_run || !gtk_file_chooser_get_filename || !gtk_file_chooser_get_filenames ||
           !gtk_file_chooser_set_do_overwrite_confirmation || !gtk_file_chooser_set_select_multiple || !gtk_file_chooser_set_current_name ||
           !gtk_file_chooser_set_filter || !gtk_file_filter_add_mime_type || !gtk_widget_destroy || !g_free_utox) {
            debug("bad GTK\n");
            dlclose(lib);
        } else {
            gtk_init(NULL, NULL);
            return lib;
        }
    }
    return NULL;
}
