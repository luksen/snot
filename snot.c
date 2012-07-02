#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <dbus/dbus.h>


// server information
const char *snot_name = "Snot";
const char *snot_vendor = "sleunk";
const char *snot_version = "0.01";
const char *snot_spec_version = "1.2";
// server capabilities
#define N_CAPS 1
const char *snot_capabilities[N_CAPS] = { "body" };


/*
 * unique IDs
 */
int snot_id() {
    static int id = 1;
    return ++id;
}


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
int snot_fifo_add(struct snot_fifo **fifo, char *app_name, char *summary, 
        char *body, int timeout);
int snot_fifo_size(struct snot_fifo *fifo);
void snot_fifo_print_top(struct snot_fifo *fifo, const char *fmt);


void snot_fifo_cut(struct snot_fifo **fifo) {
    if (snot_fifo_size(*fifo) > 0) {
        struct snot_fifo *tmp = (*fifo)->next;
        free((*fifo)->app_name);
        free((*fifo)->summary);
        free((*fifo)->body);
        free(*fifo);
        *fifo = tmp;
    }
}

int snot_fifo_add(struct snot_fifo **fifo, char *app_name, 
        char *summary, char *body, int timeout) {
    struct snot_fifo *new = malloc(sizeof(struct snot_fifo));
    new->id = snot_id();
    new->app_name = malloc(strlen(app_name) + 1);
    new->summary = malloc(strlen(summary) + 1);
    new->body = malloc(strlen(body) + 1);
    strcpy(new->app_name, app_name);
    strcpy(new->summary, summary);
    strcpy(new->body, body);
    new->timeout = timeout;
    new->next = NULL;
    if (*fifo == NULL) {
        *fifo = new;
    } 
    else {
        struct snot_fifo *tmp = *fifo;
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = new;
    }
    return new->id;
}

int snot_fifo_size(struct snot_fifo *fifo) {
    int size = 0;
    while (fifo != NULL) {
        fifo = fifo->next;
        size++;
    }
    return size;
}

void snot_fifo_print_top(struct snot_fifo *fifo, const char *fmt) {
    if (fifo != NULL) {
        char c;
        for (int i = 0; i < strlen(fmt); i++)  {
            c = fmt[i];
            if (c == '%') {
                i++;
                c = fmt[i];
                switch (c) {
                    case 'n':
                        printf("%s", fifo->app_name);
                        break;
                    case 's':
                        printf("%s", fifo->summary);
                        break;
                    case 'b':
                        printf("%s", fifo->body);
                        break;
                    case 'q':
                        printf("%d", snot_fifo_size(fifo));
                        break;
                }
            }
            else putchar(c);
        }
    }
    printf("\n");
    fflush(stdout);
}


// dbus object handler
DBusHandlerResult snot_handler(DBusConnection *conn, 
        DBusMessage *msg, void *user_data);
// exposed methods
DBusMessage* snot_get_server_information(DBusMessage *msg);
DBusMessage* snot_notify(DBusMessage *msg, struct snot_fifo **fifo);
DBusMessage* snot_get_capabilities(DBusMessage *msg);


/*
 * main
 */
int main(int args, char **argv) {
    // initialise local message buffer
    struct snot_fifo *nots;
    nots = NULL;
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
            snot_handler_vt, &nots);
    int block = -1;
    time_t expire;
    time(&expire);
    time_t last_print = time(NULL);
    while (dbus_connection_read_write_dispatch(conn, block)) {
        if (nots != NULL && block == -1) {
            block = 1000;
            time(&expire);
            expire += (time_t)(nots->timeout);
        }
        else if (nots != NULL && expire <= time(NULL)) {
            snot_fifo_cut(&nots);
            if (nots != NULL) {
                block = 1000;
                time(&expire);
                expire += (time_t)(nots->timeout);
            }
            else {
                block = -1;
                printf("\n");
                fflush(stdout);
                time(&last_print);
            }
        }
        if (nots != NULL)
            if (time(NULL) - last_print >= 1) {
                snot_fifo_print_top(nots, "[%n] %s: %b [%q]");
                time(&last_print);
            }
    }
    free(snot_handler_vt);
}


/*
 * Map DBus Method calls to functions and send back their replies.
 */
DBusHandlerResult 
snot_handler(DBusConnection *conn, DBusMessage *msg, void *fifo ) {
    DBusMessage *reply;
    const char *member = dbus_message_get_member(msg);
    //printf("handler %s\n", member);
    if (strcmp(member, "GetServerInformation") == 0) {
        reply = snot_get_server_information(msg);
        dbus_connection_send(conn, reply, NULL);
    }
    else if (strcmp(member, "Notify") == 0) {
        reply = snot_notify(msg, fifo);
        dbus_connection_send(conn, reply, NULL);
    }
    else if (strcmp(member, "GetCapabilities") == 0) {
        reply = snot_get_capabilities(msg);
        dbus_connection_send(conn, reply, NULL);
    }
    dbus_message_unref(reply);
    dbus_connection_flush(conn);
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
snot_notify(DBusMessage *msg, struct snot_fifo **fifo) {
    DBusMessage *reply;
    DBusMessageIter args;
    char *app_name;
    int replaces_id;
    char *summary;
    char *body;
    int timeout;
    int return_id;
    
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
        dbus_message_iter_get_basic(&args, &timeout);
        return_id = snot_fifo_add(fifo, app_name, summary, body, timeout);
    
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
