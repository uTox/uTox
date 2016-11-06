#ifdef UNITY
#include <messaging-menu/messaging-menu.h>
#include <unity.h>

MessagingMenuApp *  mmapp;
UnityLauncherEntry *launcher;
GMainLoop *         mmloop;
bool                unity_running;

char          f_name_data[TOX_MAX_NAME_LENGTH]                   = "";
char          f_id_data[TOX_PUBLIC_KEY_SIZE * 2 + 1]             = "";
char          f_id_data_on_minimize[TOX_PUBLIC_KEY_SIZE * 2 + 1] = "";
uint_fast32_t unread_friends                                     = 0;

// Checks if the current desktop is unity
bool is_unity_running() {
    if (strcmp(getenv("XDG_CURRENT_DESKTOP"), "Unity") == 0) {
        return 1;
    } else {
        return 0;
    }
}

// Runs the main event loop
void run_mmloop() { g_main_loop_run(mmloop); }

// Function called once the user presses an entry in the MessagingMenu
static void source_activated(MessagingMenuApp *mmapp_, const gchar *source_id, gpointer user_data) {
    // TODO
}

// Sets the user status in the Messaging Menu
void mm_set_status(int status) {
    switch (status) {

        case 0: messaging_menu_app_set_status(mmapp, MESSAGING_MENU_STATUS_AVAILABLE); break;

        case 1: messaging_menu_app_set_status(mmapp, MESSAGING_MENU_STATUS_AWAY); break;

        case 2: messaging_menu_app_set_status(mmapp, MESSAGING_MENU_STATUS_BUSY); break;
    }
}

// Function called once the user changes its status in the MessagingMenu
static void status_changed(MessagingMenuApp *mmapp_, gint status, gpointer user_data) {
    switch (status) {
        case MESSAGING_MENU_STATUS_AVAILABLE:
            self.status = 0;
            postmessage_toxcore(TOX_SETSTATUS, 0, 0, NULL);
            break;

        case MESSAGING_MENU_STATUS_AWAY:
            self.status = 1;
            postmessage_toxcore(TOX_SETSTATUS, 1, 0, NULL);
            break;

        case MESSAGING_MENU_STATUS_BUSY:
            self.status = 2;
            postmessage_toxcore(TOX_SETSTATUS, 2, 0, NULL);
            break;

        default:
            self.status = 1;
            postmessage_toxcore(TOX_SETSTATUS, 1, 0, NULL);
            break;
    }

    drawalpha(BM_ONLINE + status, SELF_STATUS_X + BM_STATUSAREA_WIDTH / 2 - BM_STATUS_WIDTH / 2,
              SELF_STATUS_Y + BM_STATUSAREA_HEIGHT / 2 - BM_STATUS_WIDTH / 2, BM_STATUS_WIDTH, BM_STATUS_WIDTH,
              status_color[status]);
}

// Registers the app in the Unity Messaging Menu
void mm_register() {
    mmapp    = messaging_menu_app_new("utox.desktop");
    launcher = unity_launcher_entry_get_for_desktop_id("utox.desktop");
    messaging_menu_app_register(mmapp);
    g_signal_connect(mmapp, "activate-source", G_CALLBACK(source_activated), NULL);
    g_signal_connect(mmapp, "status-changed", G_CALLBACK(status_changed), NULL);
    mmloop = g_main_loop_new(NULL, FALSE);
    thread(run_mmloop, NULL);
}

// Unregisters the app from the Unity Messaging Menu
void mm_unregister() {
    messaging_menu_app_unregister(mmapp);
    g_object_unref(mmapp);
    g_main_loop_unref(mmloop);
}

// Saves the current user ID when minimized
void mm_save_cid() { strcpy((char *)f_id_data_on_minimize, (char *)f_id_data); }

// Checks if a user is in the Messaging Menu
bool is_in_mm(uint8_t *f_id) {
    if (f_id == NULL) {
        strcpy((char *)f_id_data, (char *)f_id_data_on_minimize);
    } else {
        cid_to_string(f_id_data, f_id);
        f_id_data[TOX_PUBLIC_KEY_SIZE * 2] = '\0';
    }

    if (f_id_data[0] != '\0') {
        if (messaging_menu_app_has_source(mmapp, (gchar *)f_id_data)) {
            return 1;
        }
    }

    return 0;
}

// Adds an entry to the MessagingMenu
gboolean add_source() {
    messaging_menu_app_append_source(mmapp, (gchar *)f_id_data, NULL, (gchar *)f_name_data);
    messaging_menu_app_draw_attention(mmapp, (gchar *)f_id_data);
    unread_friends++;
    unity_launcher_entry_set_count(launcher, unread_friends);
    if (unread_friends == 1) {
        unity_launcher_entry_set_count_visible(launcher, TRUE);
    }
    return FALSE;
}

// Adds a new notification to the Messaging Menu.
void mm_notify(char *f_name, uint8_t *f_id) {
    if (!is_in_mm(f_id)) {
        strncpy((char *)f_name_data, (char *)f_name, TOX_MAX_NAME_LENGTH);
        g_idle_add(add_source, NULL);
    }
}

// Removes a source from the MessagingMenu
gboolean remove_source() {
    messaging_menu_app_remove_source(mmapp, (gchar *)f_id_data);
    unread_friends--;
    unity_launcher_entry_set_count(launcher, unread_friends);
    if (unread_friends == 0) {
        unity_launcher_entry_set_count_visible(launcher, FALSE);
    }
    return FALSE;
}

// Removes a notification from the Messaging Menu.
void mm_rm_entry(uint8_t *f_id) {
    if (is_in_mm(f_id)) {
        g_idle_add(remove_source, NULL);
    }
}

#endif
