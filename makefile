CC=gcc
CFLAGS= -O4

all: gbemu

main.o: main.c cpu.h rom.h opcodes.h
	$(CC) -c $(CFLAGS) $< -o $@
cpu.o: cpu.c cpu.h
	$(CC) -c $(CFLAGS) $< -o $@
opcodes.o: opcodes.c opcodes.h cpu.c
	$(CC) -c $(CFLAGS) $< -o $@
rom.o: rom.c rom.h
	$(CC) -c $(CFLAGS) $< -o $@

gbemu: main.o cpu.o rom.o opcodes.o
	$(CC) $(CFLAGS) $^ -o $@

# Target: clean project.
.PHONY: clean
clean: 
	-$(DEL) *.o
