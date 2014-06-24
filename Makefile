CFLAGS = $(shell pkg-config --cflags freetype2 x11 xft openal)
CFLAGS += -g -pthread -std=gnu99 -DV4L
LDFLAGS = $(shell pkg-config --libs freetype2 x11 xft openal)
LDFLAGS += -ltoxcore -ltoxav -ltoxdns -lresolv -ldl -lm -lfontconfig -lXrender -lv4lconvert
DESTDIR=/usr/local

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

all: uTox

uTox: $(OBJ)
	$(CC) $(CFLAGS) -o uTox $(OBJ) $(LDFLAGS)

install: uTox
	mkdir -pv $(DESTDIR)/bin
	install -m 0755 uTox $(DESTDIR)/bin

main.o: xlib/main.c xlib/keysym2ucs.c

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f uTox *.o

.PHONY: all clean
