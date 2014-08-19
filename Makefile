CFLAGS += $(shell pkg-config --cflags freetype2 x11 openal)
CFLAGS += $(shell pkg-config --cflags dbus-1)
CFLAGS += -g -pthread -std=gnu99
LDFLAGS += $(shell pkg-config --libs freetype2 x11 openal)
LDFLAGS += $(shell pkg-config --libs dbus-1)
LDFLAGS += -lX11 -lXft -lXrender -ltoxcore -ltoxav -ltoxdns -lopenal -pthread -lresolv -ldl -lm -lfontconfig -lv4lconvert -lvpx -lXext

DESTDIR?=	# empty
PREFIX?=	/usr/local

SRC = $(wildcard *.c png/png.c)
OBJ = $(SRC:.c=.o)

all: utox

utox: $(OBJ)
	$(CC) $(CFLAGS) -o utox $(OBJ) $(LDFLAGS)

install: utox
	mkdir -pv $(DESTDIR)$(PREFIX)/bin
	install -m 0755 utox $(DESTDIR)$(PREFIX)/bin

main.o: xlib/main.c xlib/keysym2ucs.c

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f utox *.o png/*.o

.PHONY: all clean
