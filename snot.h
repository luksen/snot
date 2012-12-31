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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dbus/dbus.h>
#include <errno.h>


/*
 * global configuration with defaults
 */
struct snot_config {
    int timeout;
    char *format;
    int single;
    int raw;
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
 * print error message and exit
 */
void die(char *fmt, ...);

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
static int snot_fifo_cut(struct snot_fifo **fifo);
static int snot_fifo_add(struct snot_fifo **fifo, char *app_name, char *summary,
        char *body, int timeout);
static int snot_fifo_remove(struct snot_fifo **fifo, int id); 
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
static DBusMessage* snot_close_notification(DBusMessage *msg,
        DBusConnection *conn, struct snot_fifo **fifo);
static void snot_signal_notification_closed(DBusConnection* conn, int id, 
        int reason);
