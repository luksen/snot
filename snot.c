#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dbus/dbus.h>


// server information
const char *snot_name = "Snot";
const char *snot_vendor = "sleunk";
const char *snot_version = "0.01";
const char *snot_spec_version = "1.2";
// server capabilities
#define N_CAPS 0
const char *snot_capabilities[N_CAPS] = { };

// global message counter to provide unique ids
int snot_next_id = 1;


/*
 * FIFO buffer data structure and helper functions to save notifications.
 */
struct snot_fifo{
    int id;
    char *app_name;
    char *summary;
    char *body;
    int timeout;
    struct snot_fifo *next;
};

void snot_fifo_cut(struct snot_fifo **fifo);
void snot_fifo_add(struct snot_fifo *fifo, int id, char *app_name, 
        char *summary, char *body, int timeout);
int snot_fifo_size(struct snot_fifo *fifo);
void snot_fifo_print_top(struct snot_fifo *fifo);


void snot_fifo_cut(struct snot_fifo **fifo) {
    if (snot_fifo_size(*fifo) > 1) {
        *fifo = (*fifo)->next;
    }
}

void snot_fifo_add(struct snot_fifo *fifo, int id, char *app_name, 
        char *summary, char *body, int timeout) {
    while (fifo->next != NULL) {
        fifo = fifo->next;
    }
    struct snot_fifo *new = malloc(sizeof(struct snot_fifo));
    new->id = id;
    new->app_name = app_name;
    new->summary = summary;
    new->body = body;
    new->timeout = timeout;
    new->next = NULL;
    fifo->next = new; 
}

int snot_fifo_size(struct snot_fifo *fifo) {
    int size = 0;
    while (fifo->next != NULL) {
        fifo = fifo->next;
        size++;
    }
    return size;
}

void snot_fifo_print_top(struct snot_fifo *fifo) {
    printf("[%s] %s: %s [%d]\n", fifo->app_name, fifo->summary, fifo->body, 
            snot_fifo_size(fifo));
}


// dbus object handler
DBusHandlerResult snot_handler(DBusConnection *conn, 
        DBusMessage *msg, void *user_data);
// exposed methods
DBusMessage* snot_get_server_information(DBusMessage *msg);
DBusMessage* snot_notify(DBusMessage *msg, struct snot_fifo *fifo);
DBusMessage* snot_get_capabilities(DBusMessage *msg);


/*
 * main
 */
int main(int args, int **argv) {
    // initialise local message buffer
    struct snot_fifo *nots = malloc(sizeof(struct snot_fifo));
    nots->id = 0;
    nots->app_name = "";
    nots->summary = "";
    nots->body = "";
    nots->timeout = 0;
    nots->next = NULL;
    // essential dbus vars
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
            snot_handler_vt, nots);
    while (dbus_connection_read_write_dispatch(conn, -1)) {
        snot_fifo_print_top(nots);
        snot_fifo_cut(&nots);
    }
}


/*
 * Map DBus Method calls to functions and send back their replies.
 */
DBusHandlerResult 
snot_handler(DBusConnection *conn, DBusMessage *msg, void *fifo ) {
    const char *member = dbus_message_get_member(msg);
    if (strcmp(member, "GetServerInformation") == 0) {
        dbus_connection_send(conn, snot_get_server_information(msg), NULL);
    }
    else if (strcmp(member, "Notify") == 0) {
        dbus_connection_send(conn, snot_notify(msg, fifo), NULL);
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
snot_notify(DBusMessage *msg, struct snot_fifo *fifo) {
    DBusMessage *reply;
    DBusMessageIter args;
    char *app_name;
    int replaces_id;
    char *summary;
    char *body;
    int expire_timeout;
    
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
        // skip app_icon
        dbus_message_iter_next(&args);
        dbus_message_iter_get_basic(&args, &summary);
        dbus_message_iter_next(&args);
        dbus_message_iter_get_basic(&args, &body);
        dbus_message_iter_next(&args);
        // skip actions
        dbus_message_iter_next(&args);
        // skip hints
        dbus_message_iter_next(&args);
        dbus_message_iter_get_basic(&args, &expire_timeout);
        snot_fifo_add(fifo, return_id, app_name, summary, body, expire_timeout);
    
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
snot_get_capabilities(DBusMessage *msg) {
    DBusMessage *reply;
    DBusMessageIter args;
    DBusMessageIter cap_array;
    
    // compose reply
    reply = dbus_message_new_method_return(msg);
    dbus_message_iter_init_append(reply, &args);
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &cap_array);
    // set up array
    for (int i = N_CAPS; i-->0;) {
        dbus_message_iter_append_basic(&cap_array, DBUS_TYPE_STRING, &snot_capabilities[0]);
    }
    

    dbus_message_iter_close_container(&args, &cap_array);

    return reply;
}
