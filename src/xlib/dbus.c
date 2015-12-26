#include "../main.h"

#ifndef NO_DBUS
#define HAVE_DBUS

#include <dbus/dbus.h>
#include <signal.h>

#define NOTIFY_OBJECT "/org/freedesktop/Notifications"
#define NOTIFY_INTERFACE "org.freedesktop.Notifications"

static sig_atomic_t  done;

static int notify_build_message(DBusMessage* notify_msg, char *title, char *content, uint8_t *cid)
{
    DBusMessageIter args[4];
    char *app_name = "uTox";
    uint32_t replaces_id = -1;
    char_t app_icon_data[UTOX_FILE_NAME_LENGTH];
    char *app_icon = "";
    int32_t timeout = 5000;
    dbus_bool_t m = 0;
    char* key = "foo";
    int value = 42;

    // Gets the avatar of the user to be displayed in the notification
    if(cid != NULL) {
        char_t string_cid[TOX_PUBLIC_KEY_SIZE * 2];
        cid_to_string(string_cid, cid);
        get_avatar_location(app_icon_data, string_cid);
        app_icon = (char*) app_icon_data;
    }

    dbus_message_iter_init_append(notify_msg, &args[0]);
    m |= dbus_message_iter_append_basic(&args[0], DBUS_TYPE_STRING, &app_name);
    m |= dbus_message_iter_append_basic(&args[0], DBUS_TYPE_UINT32, &replaces_id);
    m |= dbus_message_iter_append_basic(&args[0], DBUS_TYPE_STRING, &app_icon);
    m |= dbus_message_iter_append_basic(&args[0], DBUS_TYPE_STRING, &title);
    m |= dbus_message_iter_append_basic(&args[0], DBUS_TYPE_STRING, &content);

    m |= dbus_message_iter_open_container(&args[0],DBUS_TYPE_ARRAY,DBUS_TYPE_STRING_AS_STRING,&args[1]);
    /*for (i = 0; array[i]; i++ )
           m |= dbus_message_iter_append_basic(&args[1], DBUS_TYPE_STRING, &array[i]);*/
    m |= dbus_message_iter_close_container(&args[0],&args[1]);

    m |= dbus_message_iter_open_container(&args[0],DBUS_TYPE_ARRAY,"{sv}",&args[1]); /* usually {sv} for dictionaries */
    m |= dbus_message_iter_open_container(&args[1],DBUS_TYPE_DICT_ENTRY,NULL,&args[2]);
    m |= dbus_message_iter_append_basic(&args[2],DBUS_TYPE_STRING,&key);
    m |= dbus_message_iter_open_container(&args[2],DBUS_TYPE_VARIANT,DBUS_TYPE_INT32_AS_STRING,&args[3]);
    m |= dbus_message_iter_append_basic(&args[3],DBUS_TYPE_INT32,&value);
    m |= dbus_message_iter_close_container(&args[2],&args[3]);
    m |= dbus_message_iter_close_container(&args[1],&args[2]);
    m |= dbus_message_iter_close_container(&args[0],&args[1]);
    m |= dbus_message_iter_append_basic(&args[0], DBUS_TYPE_INT32, &timeout);

    return m;
}

static void notify_callback(DBusPendingCall* pending, void* user_data)
{
    done = 1;
}

void dbus_notify(char *title, char *content, uint8_t *cid)
{
    DBusMessage *msg;
    DBusConnection* conn;
    DBusError err;
    DBusPendingCall* pending;

    dbus_error_init(&err);
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);

    if(dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
    }

    if(!conn) {
        return;
    }

    msg = dbus_message_new_method_call(NULL, NOTIFY_OBJECT, NOTIFY_INTERFACE, "Notify");

    if(!msg) {
        //fprintf(stderr, "Message Null\n");
        //exit(1);
        return;
    }

    dbus_message_set_auto_start(msg, TRUE);
    dbus_message_set_destination(msg, NOTIFY_INTERFACE);

    /* append arguments
    UINT32 org.freedesktop.Notifications.Notify (STRING app_name, UINT32 replaces_id, STRING app_icon, STRING summary, STRING body, ARRAY actions, DICT hints, INT32 expire_timeout); */

    if(!notify_build_message(msg, title, content, cid)) {
        //fprintf(stderr, "Out Of Memory!\n");
        return;
    }

    dbus_error_init(&err);

    if(!dbus_connection_send_with_reply(conn, msg, &pending, -1)) {
        fprintf(stderr, "Sending failed!\n");
        exit(1);
    }

    if(!dbus_pending_call_set_notify(pending, &notify_callback, NULL, NULL)) {
        //fprintf(stderr, "Callback failed!");
        return;
    }

    while(!done) {
        dbus_connection_read_write_dispatch(conn, -1);
    }

    dbus_message_unref(msg);
    dbus_connection_unref(conn);

    return;
}

#endif
