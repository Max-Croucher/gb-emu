CC=gcc
CFLAGS= -O4

all: gbemu

main.o: main.c cpu.h rom.h
	$(CC) -c $(CFLAGS) $< -o $@
cpu.o: cpu.c cpu.h
	$(CC) -c $(CFLAGS) $< -o $@
rom.o: rom.c rom.h
	$(CC) -c $(CFLAGS) $< -o $@

gbemu: main.o cpu.o rom.o
	$(CC) $(CFLAGS) $^ -o $@

# Target: clean project.
.PHONY: clean
clean: 
	-$(DEL) *.o
