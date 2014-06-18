CFLAGS = $(shell pkg-config --cflags freetype2 x11 xft openal)
CFLAGS += -g -pthread -std=gnu99
LDFLAGS = $(shell pkg-config --libs freetype2 x11 xft openal)
LDFLAGS += -ltoxcore -ltoxav -ltoxdns -lresolv -ldl -lm

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

all: uTox

uTox: $(OBJ)
	$(CC) $(CFLAGS) -o uTox $(OBJ) $(LDFLAGS)

main.o: xlib/main.c xlib/keysym2ucs.c

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f uTox *.o

.PHONY: all clean
