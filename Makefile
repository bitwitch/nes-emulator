CC=gcc
CFLAGS=-Wall -Wextra -pedantic -D_GNU_SOURCE

.PHONY: nes

nes: main.c bus.c cpu_6502.c repl.c cart.c
	$(CC) $(CFLAGS) -o nes main.c bus.c cpu_6502.c repl.c cart.c

debug: CFLAGS += -DDEBUG_LOG
debug: nes
