CFLAGS = $(shell pkg-config --cflags freetype2 x11 xft openal)
CFLAGS += -g -pthread -std=gnu99 -DV4L
LDFLAGS = $(shell pkg-config --libs freetype2 x11 xft openal)
LDFLAGS += -lX11 -lXft -lXrender -ltoxcore -ltoxav -ltoxdns -lopenal -pthread -lresolv -ldl -lm -lfontconfig -lv4lconvert -lvpx -DV4L

DESTDIR=/usr/local

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

all: uTox

uTox: $(OBJ)
	$(CC) $(CFLAGS) -o utox $(OBJ) $(LDFLAGS)

install: utox
	mkdir -pv $(DESTDIR)/bin
	install -m 0755 utox $(DESTDIR)/bin

main.o: xlib/main.c xlib/keysym2ucs.c

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f utox *.o

.PHONY: all clean
