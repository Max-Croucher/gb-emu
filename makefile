CC=gcc
CFLAGS= -O4

all: gbemu

main.o: main.c cpu.h rom.h opcodes.h graphics.h
	$(CC) -c $(CFLAGS) $< -o $@
cpu.o: cpu.c cpu.h
	$(CC) -c $(CFLAGS) $< -o $@
opcodes.o: opcodes.c opcodes.h cpu.c
	$(CC) -c $(CFLAGS) $< -o $@
rom.o: rom.c rom.h
	$(CC) -c $(CFLAGS) $< -o $@
graphics.o: graphics.c graphics.h cpu.c
	$(CC) -c $(CFLAGS) $< -o $@ -lglut -lGL

gbemu: main.o cpu.o rom.o opcodes.o graphics.o
	$(CC) $(CFLAGS) $^ -o $@ -lglut -lGL

# Target: clean project.
.PHONY: clean
clean: 
	-$(DEL) *.o
