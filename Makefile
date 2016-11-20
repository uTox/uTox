## OPTIONS ##
# set to anything else to disable them
COCOA = 1
FILTER_AUDIO = 0

CFLAGS ?= ""
LDFLAGS ?= ""
FRAMEWORKS = -framework AppKit -framework ApplicationServices -framework CoreGraphics \
    -framework OpenAL -framework Foundation -framework CoreText -framework CoreFoundation \
    -framework AVFoundation -framework CoreVideo -framework CoreMedia -framework OpenGL \
    -framework QuartzCore -framework Cocoa -lresolv
DEPS = libtoxcore libtoxav vpx libsodium

ifeq ($(FILTER_AUDIO), 1)
    DEPS += filteraudio
    CFLAGS += -DAUDIO_FILTERING
endif

CFLAGS += -g -Wall -Wshadow -Os
CFLAGS += $(shell pkg-config --cflags $(DEPS))
LDFLAGS += $(FRAMEWORKS)
LDFLAGS += -pthread -lm
LDFLAGS += $(shell pkg-config --libs $(DEPS))

DESTDIR ?=
PREFIX ?= /usr/local

SRC = $(wildcard src/*.c src/ui/*.c src/av/*.c src/cocoa/*.m)
OBJ = $(SRC:=.o)
GIT_V = $(shell git describe --abbrev=8 --dirty --always --tags)
CFLAGS += -DGIT_VERSION=\"$(GIT_V)\"

all: utox

utox: $(OBJ)
	@echo "  LD    $@"
	@$(CC) $(CFLAGS) -o utox $(OBJ) $(LDFLAGS)

utox-static: $(OBJ)
	@echo "  SL    $@"
	$(CC) $(CFLAGS) -o utox-static $(OBJ) $(shell src/cocoa/find_static.sh $(DEPS)) $(FRAMEWORKS)

utox-MainMenu.nib:
	ibtool --compile $@ src/cocoa/MainMenu.xib

utox-Info.plist:
	sh src/cocoa/make_info_plist.sh src/cocoa/Info.plist $@

utox.icns:
	iconutil --convert icns src/cocoa/utox.iconset -o ./utox.icns

uTox.app: utox utox-Info.plist utox-MainMenu.nib utox.icns
	mkdir -p uTox.app/Contents/MacOS
	mkdir -p uTox.app/Contents/Resources
	install -m 755 utox uTox.app/Contents/MacOS/utox
	install -m 644 utox-Info.plist uTox.app/Contents/Info.plist
	install -m 644 utox.icns uTox.app/Contents/Resources/uTox.icns
	install -m 644 utox-MainMenu.nib uTox.app/Contents/Resources/MainMenu.nib

uTox.dmg: utox-static uTox.app
	sh src/cocoa/make_dmg.sh

%.m.o: %.m
	@echo "  OBJC  $@"
	@$(CC) -xobjective-c $(CFLAGS) -o $@ -c $<

%.c.o: %.c
	@echo "  CC    $@"
	@$(CC) -xobjective-c $(CFLAGS) -o $@ -c $<

clean:
	rm -f utox utox-Info.plist utox-MainMenu.nib src/cocoa/*.o src/*.o .utox_info_plist.* utox-static uTox.dmg
	rm -rf uTox.app utox_dmg.*

print-%:
	@echo $($*)

.PHONY: all clean
