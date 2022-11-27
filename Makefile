CC      := gcc 
CFLAGS  := -Wall -Wextra -pedantic -Og -g -I./src $(shell sdl2-config --cflags) 
LFLAGS  := -L/usr/local/lib -lm $(shell sdl2-config --libs)
SOURCES := $(wildcard src/*.c) 
SOURCES := $(filter-out src/repl.c, $(SOURCES))

.PHONY: nes

nes: 
	$(CC) -o nes $(SOURCES) $(CFLAGS) $(LFLAGS)

debug: CFLAGS += -DDEBUG_LOG
debug: nes

profile: CFLAGS += -pg
profile: nes
