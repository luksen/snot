#include <stdlib.h>
#include <stdio.h>
#include <dbus/dbus.h>


DBusHandlerResult snot_handler(DBusConnection* conn, 
        DBusMessage* msg, void *user_data);


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
    printf(dbus_message_get_member(msg));
    return DBUS_HANDLER_RESULT_HANDLED;
}
