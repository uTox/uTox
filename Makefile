## OPTIONS ##
# set to anything else to disable them
DBUS = 1
V4LCONVERT = 1
FILTER_AUDIO = 0
UNITY = 0

DEPS = libtoxav libtoxcore openal vpx libsodium

UNAME_S := $(shell uname -s)
UNAME_O := $(shell uname -o)

CFLAGS += -g -Wall -Wshadow -pthread -std=gnu99 -fno-strict-aliasing
LDFLAGS += -pthread -lm

ifeq ($(FILTER_AUDIO), 1)
	DEPS += filteraudio
	CFLAGS += -DAUDIO_FILTERING
endif

ifeq ($(UNAME_S), Linux)
	OUT_FILE = utox

	DEPS += fontconfig freetype2 x11 xext xrender

	ifeq ($(V4LCONVERT), 1)
		DEPS += libv4lconvert
	else
		CFLAGS += -DNO_V4LCONVERT
	endif

	ifeq ($(UNITY), 1)
		DEPS += messaging-menu unity
		CFLAGS += -DUNITY
	endif

	ifeq ($(DBUS), 1)
		DEPS += dbus-1
	else
		CFLAGS += -DNO_DBUS
	endif

	PKG_CONFIG = pkg-config

	CFLAGS += $(shell $(PKG_CONFIG) --cflags $(DEPS))

	LDFLAGS += -lresolv -ldl
	LDFLAGS += $(shell $(PKG_CONFIG) --libs $(DEPS))

	OS_SRC = $(wildcard src/xlib/*.c)
	OS_OBJ = $(OS_SRC:.c=.o)

	TRAY_OBJ = icons/utox-128x128.o
	TRAY_GEN = $(LD) -r -b binary icons/utox-128x128.png -o
else ifeq ($(UNAME_O), Cygwin)
	OUT_FILE = utox.exe

	CFLAGS  += -static
	LDFLAGS += /usr/x86_64-w64-mingw32/sys-root/mingw/lib/libwinpthread.a

	PKG_CONFIG = x86_64-w64-mingw32-pkg-config
	CFLAGS  += $(shell $(PKG_CONFIG) --cflags $(DEPS))
	LDFLAGS += $(shell $(PKG_CONFIG) --libs   $(DEPS))

	LDFLAGS += -liphlpapi -lws2_32 -lgdi32 -lmsimg32 -ldnsapi -lcomdlg32
	LDFLAGS += -Wl,-subsystem,windows -lwinmm -lole32 -loleaut32 -lstrmiids

	OS_SRC = $(wildcard src/windows/*.c)
	OS_OBJ = $(OS_SRC:.c=.o)

	TRAY_OBJ = icons/icon.o
	TRAY_GEN = x86_64-w64-mingw32-windres icons/icon.rc -O coff -o
endif


DESTDIR ?=
PREFIX ?= /usr/local
DATAROOTDIR ?= $(PREFIX)/share

SRC = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h src/*/*.h langs/*.h)
OBJ = $(SRC:.c=.o)
GIT_V = $(shell git describe --abbrev=8 --dirty --always --tags)

all: utox

utox: $(OBJ) $(OS_OBJ) $(TRAY_OBJ)
	@echo "  LD    $@"
	@$(CC) $(CFLAGS) -o $(OUT_FILE) $(OBJ) $(OS_OBJ) $(TRAY_OBJ) $(LDFLAGS)

install: utox
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m 0755 utox $(DESTDIR)$(PREFIX)/bin/utox

	mkdir -p $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/14x14/apps
	install -m 644 icons/utox-14x14.png $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/14x14/apps/utox.png
	mkdir -p $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/16x16/apps
	install -m 644 icons/utox-16x16.png $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/16x16/apps/utox.png
	mkdir -p $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/22x22/apps
	install -m 644 icons/utox-22x22.png $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/22x22/apps/utox.png
	mkdir -p $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/24x24/apps
	install -m 644 icons/utox-24x24.png $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/24x24/apps/utox.png
	mkdir -p $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/32x32/apps
	install -m 644 icons/utox-32x32.png $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/32x32/apps/utox.png
	mkdir -p $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/36x36/apps
	install -m 644 icons/utox-36x36.png $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/36x36/apps/utox.png
	mkdir -p $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/48x48/apps
	install -m 644 icons/utox-48x48.png $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/48x48/apps/utox.png
	mkdir -p $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/64x64/apps
	install -m 644 icons/utox-64x64.png $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/64x64/apps/utox.png
	mkdir -p $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/72x72/apps
	install -m 644 icons/utox-72x72.png $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/72x72/apps/utox.png
	mkdir -p $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/96x96/apps
	install -m 644 icons/utox-96x96.png $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/96x96/apps/utox.png
	mkdir -p $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/128x128/apps
	install -m 644 icons/utox-128x128.png $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/128x128/apps/utox.png
	mkdir -p $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/192x192/apps
	install -m 644 icons/utox-192x192.png $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/192x192/apps/utox.png
	mkdir -p $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/256x256/apps
	install -m 644 icons/utox-256x256.png $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/256x256/apps/utox.png
	mkdir -p $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/512x512/apps
	install -m 644 icons/utox-512x512.png $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/512x512/apps/utox.png
	mkdir -p $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/scalable/apps
	install -m 644 icons/utox.svg $(DESTDIR)$(DATAROOTDIR)/icons/hicolor/scalable/apps/utox.svg

	mkdir -p $(DESTDIR)$(DATAROOTDIR)/applications
	install -m 644 src/utox.desktop $(DESTDIR)$(DATAROOTDIR)/applications/utox.desktop
	if [ "$(UNITY)" -eq "1" ]; then echo "X-MessagingMenu-UsesChatSection=true" >> $(DESTDIR)$(DATAROOTDIR)/applications/utox.desktop; fi

	mkdir -p $(DESTDIR)$(DATAROOTDIR)/man/man1
	install -m 644 src/utox.1 $(DESTDIR)$(DATAROOTDIR)/man/man1/utox.1

$(OBJ): %.o: %.c $(HEADERS)
	@echo "  CC    $@"
	@$(CC) $(CFLAGS) -o $@ -c -DGIT_VERSION=\"$(GIT_V)\" $<

$(OS_OBJ): %.o: %.c $(HEADERS)
	@echo "  CC    $@"
	@$(CC) $(CFLAGS) -o $@ -c -DGIT_VERSION=\"$(GIT_V)\" $<

$(TRAY_OBJ):
	$(TRAY_GEN) $(TRAY_OBJ)

clean:
	rm -f $(OUT_FILE) src/*.o src/icons/*.o src/xlib/*.o src/windows/*.o

.PHONY: all clean
