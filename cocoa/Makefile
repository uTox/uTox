## OPTIONS ##
# set to anything else to disable them
COCOA = 1
FILTER_AUDIO = 1

CFLAGS ?= ""
LDFLAGS ?= ""
FRAMEWORKS = -framework AppKit -framework CoreGraphics -framework OpenAL -framework Foundation \
    -framework CoreText -framework CoreFoundation -framework AVFoundation \
    -framework CoreVideo -framework CoreMedia -framework OpenGL -framework QuartzCore \
    -lresolv
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

SRC = $(wildcard *.c cocoa/*.m)
OBJ = $(SRC:=.o)

all: utox

utox: $(OBJ)
	@echo "  LD    $@"
	@$(CC) $(CFLAGS) -o utox $(OBJ) $(LDFLAGS)

utox-static: $(OBJ)
	@echo "  SL    $@"
	@$(CC) $(CFLAGS) -o utox-static $(OBJ) $(shell cocoa/find_static.sh $(DEPS)) $(FRAMEWORKS)

utox-MainMenu.nib:
	ibtool --compile $@ cocoa/MainMenu.xib

utox-Info.plist:
	sh cocoa/make_info_plist.sh cocoa/Info.plist $@

uTox.app: utox utox-Info.plist utox-MainMenu.nib
	mkdir -p uTox.app/Contents/MacOS
	mkdir -p uTox.app/Contents/Resources
	install -m 755 utox uTox.app/Contents/MacOS/utox
	install -m 644 utox-Info.plist uTox.app/Contents/Info.plist
	install -m 644 utox-MainMenu.nib uTox.app/Contents/Resources/MainMenu.nib

uTox.dmg: utox-static uTox.app
	sh cocoa/make_dmg.sh

%.m.o: %.m
	@echo "  OBJC  $@"
	@$(CC) -xobjective-c $(CFLAGS) -o $@ -c $<

%.c.o: %.c
	@echo "  CC    $@"
	@$(CC) -xobjective-c $(CFLAGS) -o $@ -c $<

clean:
	rm -f utox utox-Info.plist utox-MainMenu.nib cocoa/*.o *.o .utox_info_plist.*

print-%:
	@echo $($*)

.PHONY: all clean
