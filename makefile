CC=gcc
CFLAGS= -O4
LDFLAGS=

all: main

main: main.c rom.c rom.h
	${CC} -o gbemu ${CFLAGS} main.c rom.c ${LDFLAGS}