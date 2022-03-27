CC=gcc
CFLAGS=-Wall -Wextra -pedantic

.PHONY: nes

nes: main.c memory.c cpu_6502.c repl.c ops.c
	$(CC) $(CFLAGS) -o nes main.c memory.c cpu_6502.c repl.c ops.c
