CC=gcc
CFLAGS=-Wall -Wextra -pedantic

.PHONY: nes

nes: main.c bus.c cpu_6502.c repl.c
	$(CC) $(CFLAGS) -o nes main.c bus.c cpu_6502.c repl.c 
