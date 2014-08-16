PREFIX = /usr/local/bin

CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -Werror -D_XOPEN_SOURCE=600 -g
DBUS = $(shell pkg-config --cflags --libs dbus-1)

.PHONY: all clean install uninstall test

all: snot

snot: snot.c snot.h
	${CC} ${CFLAGS} ${DBUS} $< -o $@

clean:
	-rm snot

install: snot
	cp snot ${PREFIX}

uninstall:
	-rm ${PREFIX}/snot

test: snot
	cram -v tests/*.t
