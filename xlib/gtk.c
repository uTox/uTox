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
void (*gtk_file_chooser_set_select_multiple)(void*, _Bool);
void (*gtk_file_chooser_set_current_name)(void*, char*);
int (*gtk_dialog_run)(void*);
void* (*gtk_file_chooser_get_filename)(void*);
void* (*gtk_file_chooser_get_filenames)(void*);
void (*gtk_widget_destroy)(void*);

volatile _Bool gtk_open;

static void gtk_openthread(void *args)
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
        postmessage(OPEN_FILES, fid, 0xFFFF, out);
    }

    gtk_widget_destroy(dialog);
    while(gtk_events_pending()) {
        gtk_main_iteration();
    }

    gtk_open = 0;
}

static void gtk_savethread(void *args)
{
    MSG_FILE *file = args;
    uint16_t fid = file->progress;
    file->progress = 0;

    void *dialog = gtk_file_chooser_dialog_new("Save File", NULL, 1, "gtk-cancel", -6, "gtk-save", -3, NULL);
    char buf[sizeof(file->name) + 1];
    memcpy(buf, file->name, file->name_length);
    buf[file->name_length] = 0;
    gtk_file_chooser_set_current_name(dialog, buf);
    int result = gtk_dialog_run(dialog);
    if(result == -3) {
        char *name = gtk_file_chooser_get_filename(dialog);
        char *path = strdup(name);
        //g_free(name)

        debug("name: %s\npath: %s\n", name, path);

        postmessage(SAVE_FILE, fid, file->filenumber, path);
    }

    gtk_widget_destroy(dialog);
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
            fwrite(file->path + ((file->flags & 1) ? 4 : 0), file->size, 1, fp);
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
    thread(gtk_openthread, (void*)(size_t)((FRIEND*)sitem->data - friend));
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
        gtk_dialog_run = dlsym(lib, "gtk_dialog_run");
        gtk_file_chooser_get_filename = dlsym(lib, "gtk_file_chooser_get_filename");
        gtk_file_chooser_get_filenames = dlsym(lib, "gtk_file_chooser_get_filenames");
        gtk_file_chooser_set_select_multiple = dlsym(lib, "gtk_file_chooser_set_select_multiple");
        gtk_file_chooser_set_current_name = dlsym(lib, "gtk_file_chooser_set_current_name");
        gtk_widget_destroy = dlsym(lib, "gtk_widget_destroy");

        if(!gtk_init || !gtk_main_iteration || !gtk_events_pending || !gtk_file_chooser_dialog_new || !gtk_dialog_run || !gtk_file_chooser_get_filename ||
           !gtk_file_chooser_get_filenames || !gtk_file_chooser_set_select_multiple || !gtk_file_chooser_set_current_name || !gtk_widget_destroy) {
            debug("bad GTK\n");
            dlclose(lib);
        } else {
            gtk_init(NULL, NULL);
            return lib;
        }
    }
    return NULL;
}
