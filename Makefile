all:
	gcc -std=c99 `pkg-config --cflags --libs dbus-1` -o snot snot.c
