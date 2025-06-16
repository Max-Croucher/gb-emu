CC=gcc
CFLAGS= -O4

all: gbemu

main.o: main.c cpu.h rom.h opcodes.h graphics.h mnemonics.h registers.h miniaudio.h audio.h
	$(CC) -c $(CFLAGS) $< -o $@
cpu.o: cpu.c cpu.h rom.h registers.h
	$(CC) -c $(CFLAGS) $< -o $@
opcodes.o: opcodes.c opcodes.h cpu.h
	$(CC) -c $(CFLAGS) $< -o $@
rom.o: rom.c rom.h
	$(CC) -c $(CFLAGS) $< -o $@
graphics.o: graphics.c graphics.h cpu.h
	$(CC) -c $(CFLAGS) $< -o $@ -lglut -lGL
audio.o: audio.c audio.h miniaudio.h
	$(CC) -c $(CFLAGS) $< -o $@ -ldl -lpthread -lm

gbemu: main.o cpu.o rom.o opcodes.o graphics.o audio.o
	$(CC) $(CFLAGS) $^ -o $@ -lglut -lGL -ldl -lpthread -lm

# Target: clean project.
.PHONY: clean
clean: 
	-$(DEL) *.o
