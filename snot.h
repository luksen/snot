/*
 *	snot - a simple text based notification server for libnotify
 *	Copyright (C) 2012 Lukas Senger
 *
 *	This file is part of snot.
 *
 *	Snot is free software: you can redistribute it and/or modify it under the
 *	terms of the GNU General Public License as published by the Free Software
 *	Foundation, either version 3 of the License, or (at your option) any later
 *	version.
 *
 *	Snot is distributed in the hope that it will be useful, but WITHOUT ANY
 *	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *	FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *	details.
 *
 *	You should have received a copy of the GNU General Public License along
 *	with this program.	If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dbus/dbus.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>


/*
 * config defaults
 */
#define DEF_TIMEOUT 3000
#define DEF_FORMAT "[%a] %s%(b: %b) %(q+%q)"
#define DEF_SINGLE 0
#define DEF_RAW 0
#define DEF_DELAY 1000


/*
 * global configuration with defaults
 */
struct config {
	int timeout;
	char *format;
	int single;
	int raw;
	int delay;
};

/*
 * FIFO buffer to save notifications.
 */
struct fifo{
	int id;
	char *app_name;
	char *summary;
	char *body;
	int timeout;
	struct fifo *next;
};


/*
 * print error message and exit
 */
static void die(char *fmt, ...);

/*
 * clean up user input
 */
static void remove_markup(char *string);
static void remove_special(char *string);

/*
 * unique IDs
 */
static int next_id();

/*
 * struct timeval covinience functions
 */
static void timeval_add_msecs(struct timeval *tp, int msecs);
static int timeval_geq(struct timeval this, struct timeval other);

/*
 * config functions
 */
static void config_init();
static void config_parse_cmd(int argc, char **argv);

/*
 * fifo functions
 */
static int fifo_cut(struct fifo **fifo);
static int fifo_add(struct fifo **fifo, char *app_name, char *summary,
		char *body, int timeout);
static int fifo_remove(struct fifo **fifo, int id); 
static int fifo_size(struct fifo *fifo);
static void fifo_print_top(struct fifo *fifo, const char *fmt);

/*
 * DBus functions
 */
// dbus object handler
static DBusHandlerResult bus_handler(DBusConnection *conn, 
		DBusMessage *msg, void *user_data);
// exposed methods
static DBusMessage* bus_get_server_information(DBusMessage *msg);
static DBusMessage* bus_notify(DBusMessage *msg, struct fifo **fifo);
static DBusMessage* bus_get_capabilities(DBusMessage *msg);
static DBusMessage* bus_close_notification(DBusMessage *msg,
		DBusConnection *conn, struct fifo **fifo);
static void bus_signal_notification_closed(DBusConnection* conn, int id, 
		int reason);

/*
 * others
 */
static void print_version();
