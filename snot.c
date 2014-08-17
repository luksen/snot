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
#include "snot.h"

// server information
const char *snot_name = "Snot";
const char *snot_vendor = "luksen";
const char *snot_version = "0.04";
const char *snot_spec_version = "1.2";
// server capabilities
#define N_CAPS 1
const char *snot_capabilities[N_CAPS] = { "body" };

jmp_buf jmpbuf;

static struct config config;


/*
 *******************************************************************************
 * general
 *******************************************************************************
 */
static void die(char *fmt, ...) {
	va_list args;
	va_start(args,fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	if (errno) {
		fprintf(stderr, ":\n	");
		perror(NULL);
	}
	else fprintf(stderr, "\n");
	fflush(stderr);
	exit(1);
}

static void print_version() {
	printf("Simple notification daemon version %s\n", snot_version);
	printf("Desktop Notifications Specification version %s\n", snot_spec_version);
	exit(EXIT_SUCCESS);
}

static void print_usage() {
	printf("--timeout TIME     -t      Change default timeout.\n");
	printf("--delay TIME       -d      Specify time between printings.\n");
	printf("--format FORMAT    -f      Define output format.\n");
	printf("--raw              -r      Don't remove markup.\n");
	printf("--single           -1      Print exactly one line for each notification.\n");
	printf("--version          -v      Print version information.\n");
	printf("--help             -h      Print this help.\n");
	printf("\n");
	printf("TIME:\n");
	printf("A time interval in milliseconds.\n");
	printf("\n");
	printf("FORMAT:\n");
	printf("These substitions are supported:\n");
	printf("  %%%%       %%\n");
	printf("  %%a       application name\n");
	printf("  %%s       notification summary\n");
	printf("  %%b       notification body\n");
	printf("  %%q       size of queue\n");
	printf("  %%(x...)  only display ... if x exists; x may be one of the above\n");
	exit(EXIT_SUCCESS);
}

static void remove_markup(char *string) {
	int lshift = 0;
	for (int i = 0; i < strlen(string); i++) {
		if (string[i] == '<' && !lshift)
			lshift = 1;
		if (lshift) {
			if (string[i] == '>') {
				memmove(string + i - lshift + 1, string + i + 1,
						strlen(string + i));
				i -= lshift;
				lshift = 0;
			}
			else lshift++;
			continue;
		}
	}
}

static void remove_special(char *string) {
	for (char *c = string; *c != '\0'; c++)
		if (*c < 32 || *c == 127) *c = ' ';
}

static int next_id() {
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

static void config_init() {
	config.timeout = DEF_TIMEOUT;
	config.format = malloc(strlen(DEF_FORMAT) + 1);
	strcpy(config.format, DEF_FORMAT);
	config.single = DEF_SINGLE;
	config.raw = DEF_RAW;
	config.delay = DEF_DELAY;
	config.newline = DEF_NEWLINE;
}

static void config_parse_cmd(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if ((argv[i][1] == 't') || (!strcmp(argv[i], "--timeout"))) {
				if (i == argc - 1)
					die("Missing argument: %s", argv[i]);
				i++;
				if (!sscanf(argv[i], "%d", &config.timeout))
					die("Timeout must be given in milliseconds");
				if (config.timeout < 0)
					die("Timeout can't be negative");
			}
			else if ((argv[i][1] == 'f') || (!strcmp(argv[i], "--format"))) {
				if (i == argc - 1)
					die("Missing argument: %s", argv[i]);
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
			else if ((argv[i][1] == 'r') || (!strcmp(argv[i], "--raw"))) {
				config.raw = 1;
			}
			else if ((argv[i][1] == 'v') || (!strcmp(argv[i], "--version"))) {
				print_version();
			}
			else if ((argv[i][1] == 'd') || (!strcmp(argv[i], "--delay"))) {
				if (i == argc - 1)
					die("Missing argument: %s", argv[i]);
				i++;
				if (!sscanf(argv[i], "%d", &config.delay))
					die("Delay must be given in milliseconds");
				if (config.delay < 0)
					die("Delay can't be negative");
			}
			else if ((argv[i][1] == 'h') || (!strcmp(argv[i], "--help"))) {
				print_usage();
			}
			else if ((argv[i][1] == 'n') || (!strcmp(argv[i], "--newline"))) {
				config.newline = 1;
			}
			else if ((argv[i][1] != '-') && (argv[i][2] != '\0')) {
				die("Short options may not be concatenated: %s", argv[i]);
			}
			else {
				die("Unknown option: %s", argv[i]);
			}
		}
		else {
			die("Argument without an option: %s", argv[i]);
		}
	}  
}


/*
 *******************************************************************************
 * fifo
 *******************************************************************************
 */
static int fifo_cut(struct fifo **fifo) {
	int id = (*fifo)->id;
	if (fifo_size(*fifo) > 0) {
		struct fifo *tmp = (*fifo)->next;
		free((*fifo)->app_name);
		free((*fifo)->summary);
		free((*fifo)->body);
		free(*fifo);
		*fifo = tmp;
	}
	return id;
}

static int fifo_add(struct fifo **fifo, char *app_name, 
		char *summary, char *body, int timeout) {
	struct fifo *new = malloc(sizeof(struct fifo));
	new->id = next_id();
	new->app_name = malloc(strlen(app_name) + 1);
	new->summary = malloc(strlen(summary) + 1);
	if (!config.raw) {
		remove_markup(body);
		remove_special(body);
	}
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
		struct fifo *tmp = *fifo;
		while (tmp->next != NULL) {
			tmp = tmp->next;
		}
		tmp->next = new;
	}
	return new->id;
}

static int fifo_remove(struct fifo **fifo, int id) {
	struct fifo *tmp = *fifo;
	int removed_sth = 0;
	while (tmp) {
		if (tmp->id == id || !id) {
			*fifo = (*fifo)->next;
			removed_sth = 1;
			break;
		}
		if (tmp->next && tmp->next->id == id) {
			struct fifo* save= tmp->next;
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

static int fifo_size(struct fifo *fifo) {
	int size = 0;
	while (fifo != NULL) {
		fifo = fifo->next;
		size++;
	}
	return size;
}

static void fifo_print_top(struct fifo *fifo, const char *fmt) {
	if (fifo != NULL) {
		char c;
		char cond = '\0';
		for (int i = 0; i < strlen(fmt); i++)  {
			c = fmt[i];
			if (cond && c == ')' && fmt[i - 1] != '%') {
				cond = '\0';
				continue;
			}
			switch (cond) {
				case 'a':
					if (!fifo->app_name[0]) continue;
					break;
				case 's':
					if (!fifo->summary[0]) continue;
					break;
				case 'b':
					if (!fifo->body[0]) continue;
					break;
				case 'q':
					if (!(fifo_size(fifo) - 1)) continue;
					break;
			}
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
						printf("%d", fifo_size(fifo) - 1);
						break;
					case '(':
						cond = fmt[++i];
						break;
					case '%':
						putchar('%');
						break;
					default:
						continue;

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
static DBusHandlerResult bus_handler(DBusConnection *conn, DBusMessage *msg, 
		void *fifo ) {
	DBusMessage *reply;
	const char *member = dbus_message_get_member(msg);
	//printf("handler %s\n", member);
	if (strcmp(member, "GetServerInformation") == 0) {
		reply = bus_get_server_information(msg);
		dbus_connection_send(conn, reply, NULL);
	}
	else if (strcmp(member, "Notify") == 0) {
		reply = bus_notify(msg, fifo);
		dbus_connection_send(conn, reply, NULL);
	}
	else if (strcmp(member, "GetCapabilities") == 0) {
		reply = bus_get_capabilities(msg);
		dbus_connection_send(conn, reply, NULL);
	}
	else if (strcmp(member, "CloseNotification") == 0) {
		reply = bus_close_notification(msg, conn, fifo);
		dbus_connection_send(conn, reply, NULL);
	}
	if (reply)
		dbus_message_unref(reply);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusMessage* bus_get_server_information(DBusMessage *msg) {
	DBusMessage *reply;
	DBusMessageIter args;

	// compose reply
	reply = dbus_message_new_method_return(msg);
	dbus_message_iter_init_append(reply, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &snot_name))
		return NULL;
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &snot_vendor))
		return NULL;
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &snot_version))
		return NULL;
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, 
				&snot_spec_version))
		return NULL;

	return reply;
}

static DBusMessage* bus_notify(DBusMessage *msg, struct fifo **fifo) {
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
		return_id = fifo_add(fifo, app_name, summary, body, timeout);
	
	// compose reply
	reply = dbus_message_new_method_return(msg);
	dbus_message_iter_init_append(reply, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &return_id))
		return NULL;

	return reply;
}

static DBusMessage* bus_get_capabilities(DBusMessage *msg) {
	DBusMessage *reply;
	DBusMessageIter args;
	DBusMessageIter cap_array;
	
	// compose reply
	reply = dbus_message_new_method_return(msg);
	dbus_message_iter_init_append(reply, &args);
	dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &cap_array);
	// set up array
	for (int i = N_CAPS; i-->0;) {
		if (!dbus_message_iter_append_basic(&cap_array, DBUS_TYPE_STRING, 
					&snot_capabilities[0]))
			return NULL;
	}
	dbus_message_iter_close_container(&args, &cap_array);

	return reply;
}

static DBusMessage* bus_close_notification(DBusMessage *msg,
		DBusConnection *conn, struct fifo **fifo) {
	DBusMessage *reply;
	DBusMessageIter args;
	int id;
	
	// read the arguments
	dbus_message_iter_init(msg, &args);
	dbus_message_iter_get_basic(&args, &id);
	int removed_sth = fifo_remove(fifo, id);
	
	// compose reply
	if (removed_sth) {
		reply = dbus_message_new_method_return(msg);
		bus_signal_notification_closed(conn, id, 3);
	}
	else
		reply = dbus_message_new_error(msg, DBUS_ERROR_FAILED, NULL);
	return reply;
}

static void bus_signal_notification_closed(DBusConnection* conn, int id, int reason) {
	DBusMessageIter args;
	DBusMessage* msg;
	// create a signal & check for errors 
	msg = dbus_message_new_signal("/org/freedesktop/Notifications",
			"org.freedesktop.Notifications",
			"NotificationClosed");
	if (!msg) return;

	// append arguments onto signal
	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &id))
		return;
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &reason))
		return;

	// send the message
	dbus_connection_send(conn, msg, NULL);
	dbus_message_unref(msg);
}


void on_sigint(int sig) {
	longjmp(jmpbuf, 1);
}

/*
 *******************************************************************************
 * main
 *******************************************************************************
 */
int main(int args, char **argv) {
	// setup config
	config_init();
	config_parse_cmd(args, argv);

	// initialise local message buffer
	struct fifo *nots;
	nots = NULL;
	// essential dbus vars
	DBusError err;
	DBusConnection* conn;
	// wrap bus_handler in vtable
	DBusObjectPathVTable *bus_handler_vt = 
		malloc(sizeof(DBusObjectPathVTable));
	bus_handler_vt->message_function = &bus_handler;
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
			bus_handler_vt, &nots);
	int block = -1;
	struct timeval expire;
	gettimeofday(&expire, NULL);
	struct timeval last_print;
	struct timeval now;
	gettimeofday(&now, NULL);
	last_print.tv_sec = 0;
	last_print.tv_usec = 0;
	int printed = 0;
	
	// setup signal handler
	struct sigaction act = {
		.sa_handler = on_sigint
	};
	if (sigemptyset(&act.sa_mask))
		die("Failed to set up signal handler");
	if (setjmp(jmpbuf) || sigaction(SIGTERM, &act, NULL) ||
			(sigaction(SIGINT, &act, NULL)))
		die("Failed to set up signal handler");
	while (!setjmp(jmpbuf) && dbus_connection_read_write_dispatch(conn, block)) {
		// no new notification
		if (!nots) continue;
		// in single mode no calculations required
		if (config.single) {
			fifo_print_top(nots, config.format);
			fifo_cut(&nots);
			continue;
		}
		gettimeofday(&now, NULL);
		if (block == -1) {
			// queue was empty
			block = config.delay;
		}
		else if (printed && timeval_geq(now, expire)
				&& nots->timeout > -1
				&& (config.delay == 0
					|| printed >= nots->timeout/config.delay)) {
			// notification expired
			int id = fifo_cut(&nots);
			bus_signal_notification_closed(conn, id, 1);
			printed = 0;
			block = config.delay;
			if (!nots) {
				// no new notification
				block = -1;
				if (config.newline) {
					printf("\n");
					fflush(stdout);
					gettimeofday(&last_print, NULL);
				}
				continue;
			}
		}
		// Allow printing 200msec early
		timeval_add_msecs(&now, -config.delay);
		timeval_add_msecs(&now, 200);
		if (timeval_geq(now, last_print)) {
			fifo_print_top(nots, config.format);
			gettimeofday(&last_print, NULL);
			if (!printed) {
				gettimeofday(&expire, NULL);
				timeval_add_msecs(&expire, nots->timeout);
			}
			printed++;
		}
	}
	dbus_error_free(&err); 
	free(bus_handler_vt);
	dbus_connection_unref(conn);
}
