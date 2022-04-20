CC=gcc
CFLAGS=-Wall -Wextra -pedantic -D_GNU_SOURCE -O2 $(shell sdl2-config --cflags --libs)
LFLAGS := -L/usr/local/lib -lSDL2_mixer

.PHONY: nes

nes: main.c bus.c cpu_6502.c cart.c io.c
	$(CC) -o nes main.c bus.c cpu_6502.c cart.c io.c $(LFLAGS) $(CFLAGS) 

debug: CFLAGS += -DDEBUG_LOG
debug: nes
