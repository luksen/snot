#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dbus/dbus.h>


// server information
const char *snot_name = "Snot";
const char *snot_vendor = "sleunk";
const char *snot_version = "0.01";
const char *snot_spec_version = "1.2";

// global message counter to provide unique ids
int snot_next_id = 1;

// dbus object handler
DBusHandlerResult snot_handler(DBusConnection *conn, 
        DBusMessage *msg, void *user_data);
// exposed methods
DBusMessage* snot_get_server_information(DBusMessage *msg);
DBusMessage* snot_notify(DBusMessage *msg);
DBusMessage* snot_get_capabilities(DBusMessage *msg);


int main(int args, int **argv) {
    DBusError err;
    DBusConnection* conn;
    // wrap snot_handler in vtable
    DBusObjectPathVTable *snot_handler_vt = 
        malloc(sizeof(DBusObjectPathVTable));
    snot_handler_vt->message_function = &snot_handler;
    // initialise the errors
    dbus_error_init(&err);

    // connect to the bus
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) { 
        fprintf(stderr, "Connection Error (%s)\n", err.message); 
        dbus_error_free(&err); 
    }
    if (conn == NULL) { 
        exit(1); 
    }

    // request name on the bus
    int ret;
    ret = dbus_bus_request_name(conn, "org.freedesktop.Notifications", 
        DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (dbus_error_is_set(&err)) { 
        fprintf(stderr, "Name Error (%s)\n", err.message); 
        dbus_error_free(&err); 
    }
    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) { 
        exit(1);
    }

    // run incoming method calls through the handler
    dbus_connection_register_object_path(conn, "/org/freedesktop/Notifications", 
            snot_handler_vt, NULL);
    while (dbus_connection_read_write_dispatch(conn, -1)) {
        printf("loop\n");
    }
}


DBusHandlerResult 
snot_handler(DBusConnection *conn, DBusMessage *msg, void *user_data ) {
    const char *member = dbus_message_get_member(msg);
    printf("%s\n", member);
    if (strcmp(member, "GetServerInformation") == 0) {
        dbus_connection_send(conn, snot_get_server_information(msg), NULL);
    }
    else if (strcmp(member, "Notify") == 0) {
        dbus_connection_send(conn, snot_notify(msg), NULL);
    }
    else if (strcmp(member, "GetCapabilities") == 0) {
        dbus_connection_send(conn, snot_get_capabilities(msg), NULL);
    }
    return DBUS_HANDLER_RESULT_HANDLED;
}


DBusMessage* 
snot_get_server_information(DBusMessage *msg) {
    DBusMessage *reply;
    DBusMessageIter args;

    // compose reply
    reply = dbus_message_new_method_return(msg);
    dbus_message_iter_init_append(reply, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &snot_name)) { 
        fprintf(stderr, "Out Of Memory!\n"); 
        exit(1);
    }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, 
                &snot_vendor)) { 
        fprintf(stderr, "Out Of Memory!\n"); 
        exit(1);
    }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, 
                &snot_version)) { 
        fprintf(stderr, "Out Of Memory!\n"); 
        exit(1);
    }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, 
                &snot_spec_version)) { 
        fprintf(stderr, "Out Of Memory!\n"); 
        exit(1);
    }

    return reply;
}

DBusMessage* 
snot_notify(DBusMessage *msg) {
    DBusMessage *reply;
    DBusMessageIter args;
    char *app_name;
    int replaces_id;
    char *summary;
    //increment global message counter
    int return_id = snot_next_id++;
    
    // read the arguments
    if (!dbus_message_iter_init(msg, &args))
        fprintf(stderr, "Empty notification received.\n"); 
    else 
        dbus_message_iter_get_basic(&args, &app_name);
        dbus_message_iter_next(&args);
        dbus_message_iter_get_basic(&args, &replaces_id);
        dbus_message_iter_next(&args);
        dbus_message_iter_next(&args);
        dbus_message_iter_get_basic(&args, &summary);
        printf("%s: %s\n", app_name, summary);
    
    // compose reply
    reply = dbus_message_new_method_return(msg);
    dbus_message_iter_init_append(reply, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &return_id)) { 
        fprintf(stderr, "Out Of Memory!\n"); 
        exit(1);
    }

    return reply;
}

DBusMessage* 
snot_ge_capabilities(DBusMessage *msg) {
    DBusMessage *reply;
    DBusMessageIter args;
    DBusMessageIter cap_array;
    
    // compose reply
    reply = dbus_message_new_method_return(msg);
    dbus_message_iter_init_append(reply, &args);
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING,
            &cap_array);
    // set up array
    

    dbus_message_iter_close_container(&args, $cap_array);

    return reply;
}
