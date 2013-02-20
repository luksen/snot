CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -Werror -D_XOPEN_SOURCE=600 -g
DBUS = $(shell pkg-config --cflags --libs dbus-1)

.PHONY: all clean install

all: snot

snot: snot.c snot.h
	${CC} ${CFLAGS} ${DBUS} $< -o $@

install: snot
	cp snot /usr/bin/

clean:
	-rm snot

test: test.c snot.h snot.c
	${CC} -std=c99 ${DBUS} $< -o $@
