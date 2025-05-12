CC=gcc
CFLAGS= -O4
LDFLAGS=

all: main

main: main.c cpu.c cpu.h rom.c rom.h
	${CC} -o gbemu ${CFLAGS} main.c cpu.c rom.c ${LDFLAGS}