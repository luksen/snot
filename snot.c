/*
 *  snot - a simple text based notification server for libnotify
 *  Copyright (C) 2012 Lukas Senger
 *
 *  This file is part of snot.
 *
 *  Snot is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  Snot is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "snot.h"

// server information
const char *snot_name = "Snot";
const char *snot_vendor = "sleunk";
const char *snot_version = "0.01";
const char *snot_spec_version = "1.2";
// server capabilities
#define N_CAPS 1
const char *snot_capabilities[N_CAPS] = { "body" };


static struct snot_config config;


/*
 *******************************************************************************
 * general
 *******************************************************************************
 */
void die(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, fmt, args);
    va_end(args);
    if (errno) {
        fprintf(stderr, ":\n    ");
        perror(NULL);
    }
    else fprintf(stderr, "\n");
    fflush(stderr);
    exit(1);
}

static int snot_id() {
    static int id = 1;
    return ++id;
}

static void timeval_add_msecs(struct timeval *tp, int msecs) {
    tp->tv_sec += msecs/1000;
    tp->tv_usec += (msecs%1000)*1000;
}

static int timeval_geq(struct timeval this, struct timeval other) {
    if (this.tv_sec > other.tv_sec)
        return 1;
    if (this.tv_sec == other.tv_sec)
        if (this.tv_usec >= other.tv_usec)
            return 1;
    return 0;
}

static void snot_config_init() {
    config.timeout = 3000;
    config.format = malloc(18);
    strcpy(config.format, "[%a] %s: %b [%q]");
    config.single = 0;
}

static void snot_config_parse_cmd(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if ((argv[i][1] == 't') || (!strcmp(argv[i], "--timeout"))) {
                i++;
                sscanf(argv[i], "%d", &config.timeout);
            }
            else if ((argv[i][1] == 'f') || (!strcmp(argv[i], "--format"))) {
                i++;
                free(config.format);
                char fmt[6];
                int len = strlen(argv[i]);
                sprintf(fmt, "%%%dc", len);
                config.format = malloc(len + 1);
                sscanf(argv[i], fmt, config.format);
                config.format[len] = '\0';
            }
            else if ((argv[i][1] == '1') || (!strcmp(argv[i], "--single"))) {
                config.single = 1;
            }
        }
    }  
}


/*
 *******************************************************************************
 * fifo
 *******************************************************************************
 */
static int snot_fifo_cut(struct snot_fifo **fifo) {
    int id = (*fifo)->id;
    if (snot_fifo_size(*fifo) > 0) {
        struct snot_fifo *tmp = (*fifo)->next;
        free((*fifo)->app_name);
        free((*fifo)->summary);
        free((*fifo)->body);
        free(*fifo);
        *fifo = tmp;
    }
    return id;
}

static int snot_fifo_add(struct snot_fifo **fifo, char *app_name, 
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
    if (timeout == -1) new->timeout = config.timeout;
    if (timeout == 0) new->timeout = -1;
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

static int snot_fifo_remove(struct snot_fifo **fifo, int id) {
    struct snot_fifo *tmp = *fifo;
    int removed_sth = 0;
    while (tmp) {
        if (tmp->id == id || !id) {
            *fifo = (*fifo)->next;
            removed_sth = 1;
            break;
        }
        if (tmp->next && tmp->next->id == id) {
            struct snot_fifo* save= tmp->next;
            tmp->next = tmp->next->next;
            free(save->app_name);
            free(save->summary);
            free(save->body);
            free(save);
            removed_sth = 1;
            break;
        }
        tmp = tmp->next;
    }
    return removed_sth;
}

static int snot_fifo_size(struct snot_fifo *fifo) {
    int size = 0;
    while (fifo != NULL) {
        fifo = fifo->next;
        size++;
    }
    return size;
}

static void snot_fifo_print_top(struct snot_fifo *fifo, const char *fmt) {
    if (fifo != NULL) {
        char c;
        for (int i = 0; i < strlen(fmt); i++)  {
            c = fmt[i];
            if (c == '%') {
                i++;
                c = fmt[i];
                switch (c) {
                    case 'a':
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


/*
 *******************************************************************************
 * dbus
 *******************************************************************************
 */
static DBusHandlerResult snot_handler(DBusConnection *conn, DBusMessage *msg, 
        void *fifo ) {
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
    else if (strcmp(member, "CloseNotification") == 0) {
        reply = snot_close_notification(msg, conn, fifo);
        dbus_connection_send(conn, reply, NULL);
    }
    dbus_message_unref(reply);
    dbus_connection_flush(conn);
    return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusMessage* snot_get_server_information(DBusMessage *msg) {
    DBusMessage *reply;
    DBusMessageIter args;

    // compose reply
    reply = dbus_message_new_method_return(msg);
    dbus_message_iter_init_append(reply, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &snot_name))
        die("Could not prepare reply in %s", __func__);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &snot_vendor))
        die("Could not prepare reply in %s", __func__);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &snot_version))
        die("Could not prepare reply in %s", __func__);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, 
                &snot_spec_version))
        die("Could not prepare reply in %s", __func__);

    return reply;
}

static DBusMessage* snot_notify(DBusMessage *msg, struct snot_fifo **fifo) {
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
        // replaces_id ignored atm
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
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &return_id))
        die("Could not prepare reply in %s", __func__);

    return reply;
}

static DBusMessage* snot_get_capabilities(DBusMessage *msg) {
    DBusMessage *reply;
    DBusMessageIter args;
    DBusMessageIter cap_array;
    
    // compose reply
    reply = dbus_message_new_method_return(msg);
    dbus_message_iter_init_append(reply, &args);
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &cap_array);
    // set up array
    for (int i = N_CAPS; i-->0;) {
        if(!dbus_message_iter_append_basic(&cap_array, DBUS_TYPE_STRING, 
                    &snot_capabilities[0]))
            die("Could not prepare reply in %s", __func__);
    }
    

    dbus_message_iter_close_container(&args, &cap_array);

    return reply;
}

static DBusMessage* snot_close_notification(DBusMessage *msg,
        DBusConnection *conn, struct snot_fifo **fifo) {
    DBusMessage *reply;
    DBusMessageIter args;
    int id;
    
    // read the arguments
    dbus_message_iter_init(msg, &args);
    dbus_message_iter_get_basic(&args, &id);
    int removed_sth = snot_fifo_remove(fifo, id);
    
    // compose reply
    if (removed_sth) {
        reply = dbus_message_new_method_return(msg);
        snot_signal_notification_closed(conn, id, 3);
    }
    else
        reply = dbus_message_new_error(msg, DBUS_ERROR_FAILED, NULL);
    return reply;
}

static void snot_signal_notification_closed(DBusConnection* conn, int id, int reason) {
    DBusMessageIter args;
    DBusMessage* msg;
    // create a signal & check for errors 
    msg = dbus_message_new_signal("/org/freedesktop/Notifications",
            "org.freedesktop.Notifications",
            "NotificationClosed");
    if (!msg) die("Could not prepare signal message in %s", __func__);

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &id))
        die("Could not append argument to signal in %s", __func__);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &reason))
        die("Could not append argument to signal in %s", __func__);

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, NULL))
        die("Sending signal failed in %s", __func__);
    dbus_connection_flush(conn);
}


/*
 *******************************************************************************
 * main
 *******************************************************************************
 */
int main(int args, char **argv) {
    snot_config_init();
    snot_config_parse_cmd(args, argv);
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
    if (conn == NULL)
        die("Can't connect to the message bus");

    // request name on the bus
    int ret;
    ret = dbus_bus_request_name(conn, "org.freedesktop.Notifications", 
        DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (dbus_error_is_set(&err)) { 
        fprintf(stderr, "Name Error (%s)\n", err.message); 
        dbus_error_free(&err); 
    }
    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
        die("Another notification daemon seems to be running");

    // run incoming method calls through the handler
    dbus_connection_register_object_path(conn, "/org/freedesktop/Notifications",
            snot_handler_vt, &nots);
    int block = -1;
    struct timeval expire;
    gettimeofday(&expire, NULL);
    struct timeval last_print;
    gettimeofday(&last_print, NULL);
    struct timeval now;
    gettimeofday(&now, NULL);
    while (dbus_connection_read_write_dispatch(conn, block)) {
        if (!nots) continue;
        if (config.single) {
            snot_fifo_print_top(nots, config.format);
            snot_fifo_cut(&nots);
            continue;
        }
        gettimeofday(&now, NULL);
        if (block == -1) {
            // queue was empty
            block = 1000;
            gettimeofday(&expire, NULL);
            timeval_add_msecs(&expire, nots->timeout);
        }
        else if (timeval_geq(now, expire) && nots->timeout > -1) {
            // notification expired
            int id = snot_fifo_cut(&nots);
            snot_signal_notification_closed(conn, id, 1);
            if (nots) {
                block = 1000;
                gettimeofday(&expire, NULL);
                timeval_add_msecs(&expire, nots->timeout);
            }
            else {
                // no new notification
                block = -1;
                printf("\n");
                fflush(stdout);
                gettimeofday(&last_print, NULL);
            }
        }
        // make sure the number of lines printed equals the timeout in seconds
        now.tv_sec--;
        if (timeval_geq(now, last_print)) {
            snot_fifo_print_top(nots, config.format);
            gettimeofday(&last_print, NULL);
        }
    }
    free(snot_handler_vt);
}
