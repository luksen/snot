#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dbus/dbus.h>


/*
 * global configuration with defaults
 */
struct snot_config {
    int timeout;
    char *format;
};

/*
 * FIFO buffer to save notifications.
 */
struct snot_fifo{
    int id;
    char *app_name;
    char *summary;
    char *body;
    int timeout;
    struct snot_fifo *next;
};


/*
 * unique IDs
 */
static int snot_id();

/*
 * struct timeval covinience functions
 */
static void timeval_add_msecs(struct timeval *tp, int msecs);
static int timeval_geq(struct timeval this, struct timeval other);

/*
 * config functions
 */
static void snot_config_init();
static void snot_config_parse_cmd(int argc, char **argv);

/*
 * fifo functions
 */
static void snot_fifo_cut(struct snot_fifo **fifo);
static int snot_fifo_add(struct snot_fifo **fifo, char *app_name, char *summary,
        char *body, int timeout);
static int snot_fifo_size(struct snot_fifo *fifo);
static void snot_fifo_print_top(struct snot_fifo *fifo, const char *fmt);

/*
 * DBus functions
 */
// dbus object handler
static DBusHandlerResult snot_handler(DBusConnection *conn, 
        DBusMessage *msg, void *user_data);
// exposed methods
static DBusMessage* snot_get_server_information(DBusMessage *msg);
static DBusMessage* snot_notify(DBusMessage *msg, struct snot_fifo **fifo);
static DBusMessage* snot_get_capabilities(DBusMessage *msg);
