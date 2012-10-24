CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -Werror
DBUS = $(shell pkg-config --cflags --libs dbus-1)

.PHONY: all clean install

all: snot

snot: snot.c
	${CC} ${CFLAGS} ${DBUS} $< -o $@

install: snot
	cp snot /usr/bin/

clean:
	-rm snot
