CC = gcc
CFLAGS = -std=c99 -Wall -Werror
DBUS = $(shell pkg-config --cflags --libs dbus-1)

.PHONY: all clean

all: snot

snot: snot.c
	${CC} ${CFLAGS} ${DBUS} $< -o $@

clean:
	-rm -f snot
