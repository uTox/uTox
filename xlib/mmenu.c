MessagingMenuApp *mmapp;
GMainLoop *mmloop;
_Bool unity_running;

char_t f_name_data[TOX_MAX_NAME_LENGTH] = "";
char_t f_id_data[TOX_PUBLIC_KEY_SIZE * 2 + 1] = "";
char_t f_id_data_on_minimize[TOX_PUBLIC_KEY_SIZE * 2 + 1] = "";

// Checks if the current desktop is unity
_Bool is_unity_running()
{
    if(strcmp(getenv("XDG_CURRENT_DESKTOP"), "Unity") == 0) {
        return 1;
    }
    else {
        return 0;
    }
}

// Runs the main event loop
void run_mmloop()
{
    g_main_loop_run(mmloop);
}

// Function called once the user pressed an entry in the MessagingMenu
static void source_activated(MessagingMenuApp *mmapp_, const gchar *source_id, gpointer user_data)
{
    // TODO
}

// Registers the app in the Unity Messaging Menu
void mm_register()
{
    mmapp = messaging_menu_app_new("utox.desktop");
    messaging_menu_app_register(mmapp);
    g_signal_connect(mmapp, "activate-source", G_CALLBACK (source_activated), NULL);
    mmloop = g_main_loop_new(NULL, FALSE);
    thread(run_mmloop, NULL);
}

// Unregisters the app from the Unity Messaging Menu
void mm_unregister()
{
    messaging_menu_app_unregister(mmapp);
    g_object_unref(mmapp);
    g_main_loop_unref(mmloop);
}

// Saves the current user ID when minimized
void mm_save_cid()
{
    strcpy((char*)f_id_data_on_minimize, (char*)f_id_data);
}

// Checks if a user is in the Messaging Menu
_Bool is_in_mm(uint8_t *f_id)
{
    if(f_id == NULL) {
        strcpy((char*)f_id_data, (char*)f_id_data_on_minimize);
    }
    else {
        cid_to_string(f_id_data, f_id);
        f_id_data[TOX_PUBLIC_KEY_SIZE * 2] = '\0';
    }

    if(f_id_data[0] != '\0') {
        if(messaging_menu_app_has_source(mmapp, (gchar*)f_id_data)) {
            return 1;
        }
    }

    return 0;
}

// Adds an entry to the MessagingMenu
gboolean add_source()
{
    messaging_menu_app_append_source(mmapp, (gchar*)f_id_data, NULL, (gchar*)f_name_data);
    messaging_menu_app_draw_attention(mmapp, (gchar*)f_id_data);
    return FALSE;
}

// Adds a new notification to the Messaging Menu.
void mm_notify(char_t *f_name, uint8_t *f_id)
{
    if(!is_in_mm(f_id)) {
        strncpy((char*)f_name_data, (char*)f_name, TOX_MAX_NAME_LENGTH);
        g_idle_add(add_source, NULL);
    }
}

// Removes a source from the MessagingMenu
gboolean remove_source()
{
    messaging_menu_app_remove_source(mmapp, (gchar*)f_id_data);
    return FALSE;
}

// Removes a notification from the Messaging Menu.
void mm_rm_entry(uint8_t *f_id)
{
    if(is_in_mm(f_id)) {
        g_idle_add(remove_source, NULL);
    }
}
