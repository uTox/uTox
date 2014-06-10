CFLAGS = $(shell pkg-config --cflags freetype2 x11 xft openal)
CFLAGS += -g -pthread
LDFLAGS = $(shell pkg-config --libs freetype2 x11 xft openal)
LDFLAGS += -ltoxcore -ltoxav

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

all: winTox

winTox: $(OBJ)
	$(CC) $(CFLAGS) -o winTox $(OBJ) $(LDFLAGS)

main.o: xlib/main.c xlib/keysym2ucs.c

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f winTox *.o

.PHONY: all clean
