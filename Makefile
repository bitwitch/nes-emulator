CC      := gcc 
CFLAGS  := -Wall -Wextra -pedantic -Og -g -I./src $(shell sdl2-config --cflags) 
LFLAGS  := -L/usr/local/lib -lm $(shell sdl2-config --libs)

.PHONY: nes

nes: 
	$(CC) -o nes src/main.c $(CFLAGS) $(LFLAGS)

debug: CFLAGS += -DDEBUG_LOG
debug: nes

profile: CFLAGS += -pg
profile: nes
