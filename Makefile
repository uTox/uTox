## OPTIONS ##
# set to anything else to disable them
DBUS			= 1
V4LCONVERT		= 1
FILTER_AUDIO	= 0
UNITY			= 0
XP				= 0

UNAME_S		:= $(shell uname -s)
UNAME_O		:= $(shell uname -o)

CFLAGS		+= -g -Wall -Wshadow -pthread -std=gnu99 -fno-strict-aliasing
LDFLAGS		+= -pthread -lm

DESTDIR		?=
PREFIX		?= /usr/local
DATAROOTDIR ?= $(PREFIX)/share

HEADERS 	= $(wildcard src/*.h src/*/*.h langs/*.h)
SRC 		= $(wildcard src/*.c src/ui/*.c src/av/*.c)
OBJ 		= $(SRC:.c=.o)

GIT_V		= $(shell git describe --abbrev=8 --dirty --always --tags)
CFLAGS		+= -DGIT_VERSION=\"$(GIT_V)\"

WIN_SRC		= $(wildcard src/windows/*.c)
WIN_OBJ		= $(WIN_SRC:.c=.o)

UNX_SRC		= $(wildcard src/xlib/*.c)
UNX_OBJ		= $(UNX_SRC:.c=.o)

TRAY_OBJ	= icons/icon.o

ifeq ($(UNAME_S), Linux)
	SYS        	= Unix
	OUT_FILE 	= utox
	PKG_CONFIG 	= pkg-config

	DEPS 		= libtoxav libtoxcore openal vpx libsodium fontconfig freetype2 x11 xext xrender

	ifeq ($(V4LCONVERT), 1)
		DEPS += libv4lconvert
	else
		UNX_CFLAGS += -DNO_V4LCONVERT
	endif

	ifeq ($(UNITY), 1)
		DEPS += messaging-menu unity
		UNX_CFLAGS += -DUNITY
	endif

	ifeq ($(DBUS), 1)
		DEPS += dbus-1
		UNX_CFLAGS += -DHAVE_DBUS
	else
		UNX_CFLAGS += -DNO_DBUS
	endif

	UNX_CFLAGS += $(shell $(PKG_CONFIG) --cflags $(DEPS))
	LDFLAGS += -lresolv -ldl $(shell $(PKG_CONFIG) --libs $(DEPS))
	TRAY_GEN = $(LD) -r -b binary icons/utox-128x128.png -o

	ifeq ($(CC), x86_64-w64-mingw32-gcc)
		OUT_FILE	= uTox-x64.exe
		TRAY_GEN	= x86_64-w64-mingw32-windres icons/icon.rc -O coff -o
		CFLAGS		= $(WIN_CFLAGS)
	endif

	ifeq ($(CC), i686-w64-mingw32-gcc)
		ifeq ($(XP), 1)
			WIN_CFLAGS += -D__WIN_LEGACY
		endif

		OUT_FILE	= uTox-x32.exe
		TRAY_GEN	= i686-w64-mingw32-windres icons/icon.rc -O coff -o
		CFLAGS		= $(WIN_CFLAGS)
	endif

else ifeq ($(UNAME_O), Cygwin)
	SYS			= Cygwin
	OUT_FILE	=
	PKG_CONFIG	= x86_64-w64-mingw32-pkg-config
	CC			= x86_64-w64-mingw32-gcc
endif

WIN_CFLAGS   = -Wall -Wshadow -pthread -Ofast -std=gnu99 -fgnu89-inline -fno-strict-aliasing -DAL_LIBTYPE_STATIC -DGIT_VERSION=\"$(GIT_V)\" -I libs/windows-x64/include/
WIN_LDFLAGS += -lm -pthread -liphlpapi -lws2_32 -lgdi32 -lmsimg32 -ldnsapi -lcomdlg32 -lwinmm -lole32 -loleaut32 -lstrmiids -Wl,-subsystem,windows

STATIC_LIBS  = lib/{libOpenAL32,libtoxav,libtoxdns,libtoxcore,libtoxencryptsave,libvpx,libopus,libsodium}.a
DYNMIC_LIBS  = -L ./libs/windows-x64/lib/ -lm -lOpenAL32 -ltoxav -ltoxcore -ltoxdns -ltoxencryptsave -lopus -lvpx -lsodium


all: $(OUT_FILE)

# Cross compile recipe
uTox-x64.exe: $(SRC) $(WIN_SRC) $(TRAY_OBJ) libs-64 x64-libs
	$(eval LOCAL_LIBS=./libs/windows-x64/$(STATIC_LIBS))
	@echo "  Cross Compiling Windows x64 $@"
	@x86_64-w64-mingw32-windres icons/icon.rc -O coff -o $(TRAY_OBJ)
	@x86_64-w64-mingw32-gcc $(WIN_CFLAGS) -o $@ $(SRC) $(WIN_SRC) $(TRAY_OBJ) $(LOCAL_LIBS) $(WIN_LDFLAGS) /usr/x86_64-w64-mingw32/lib/libwinpthread.a

# Cross compile recipe
uTox-x32.exe: $(SRC) $(WIN_SRC) $(TRAY_OBJ) libs-32 x32-libs
	$(eval LOCAL_LIBS=./libs/windows-x32/$(STATIC_LIBS))
	@echo "  Cross Compiling Windows x32 $@"
	@i686-w64-mingw32-windres icons/icon.rc -O coff -o $(TRAY_OBJ)
	@i686-w64-mingw32-gcc   $(WIN_CFLAGS) -o $@ $(SRC) $(WIN_SRC) $(TRAY_OBJ) $(LOCAL_LIBS) $(WIN_LDFLAGS) /usr/i686-w64-mingw32/lib/libwinpthread.a

# Cross compile recipe
uTox-win32-winXP.exe: $(OBJ) $(WIN_OBJ) $(TRAY_OBJ) x32-libs
	@echo "  Cross Compiling Windows x32 LEGACY VERSION $@"
	@i686-w64-mingw32-gcc   $(WIN_CFLAGS) -o $@ $(SRC) $(WIN_SRC) $(TRAY_OBJ) $(STATIC_LIBS) $(WIN_LDFLAGS) /usr/i686-w64-mingw32/lib/libwinpthread.a

# Dynamiclly built & linked recipe
utox.exe utox : $(OBJ) $(UNX_OBJ) $(TRAY_OBJ)
	@echo "  LD    $@"
	@$(CC) $(CFLAGS) -o $(OUT_FILE) $(OBJ) $(UNX_OBJ) $(TRAY_OBJ) $(LDFLAGS)

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
	@echo "      CC    $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

# This will only work on Cygwin
$(WIN_OBJ): %.o: %.c $(HEADERS)
	@echo "  WIN_CC    $@"
	@$(CC) $(CFLAGS) -c -o $@ -c $<

$(UNX_OBJ): %.o: %.c $(HEADERS)
	@echo "  UNX_CC    $@"
	@$(CC) $(CFLAGS) -o $@ -c $<

$(TRAY_OBJ):
	@echo "Creating tray icon"
	@$(TRAY_GEN) $@

clean: win-clean
	rm -f ./$(OUT_FILE) ./$(TRAY_OBJ) ./src/*.o ./src/xlib/*.o

clean-all: clean win-clean libs-clean dist-clean

win-clean:
	rm -f ./src/windows/*.o ./uTox-x*.exe

libs-clean:
	rm -r ./libs/*

dist-clean:
	rm -r ./dist/*

all-libs: libs-64 x32-libs libs-64 x64-libs

libs-32:
	@echo "Fetching pre-built windows x32 libs from build.tox.chat... "
	@mkdir -p libs/windows-x32/

libs-64:
	@echo "Fetching pre-built windows x64 libs from build.tox.chat... "
	@mkdir -p libs/windows-x64/

x32-libs:  libs/libopus_build_windows_x86_static_release.zip libs/libopenal_build_windows_x86_static_release.zip libs/libtoxcore_build_windows_x86_static_release.zip libs/libvpx_build_windows_x86_static_release.zip libs/libsodium_build_windows_x86_static_release.zip libs/libfilteraudio-x86.zip
	@echo "Done!"

libs/libfilteraudio-x86.zip:
	@echo "	Fetching $@ from https://build.tox.chat"
	@cd libs/ && curl -s -O https://build.tox.chat/view/libfilteraudio/job/libfilteraudio_build_windows_x86_static_release/lastSuccessfulBuild/artifact/libfilteraudio.zip
	@mv ./libs/libfilteraudio.zip ./libs/libfilteraudio-x86.zip
	@unzip -qq -d libs/windows-x32/ $@

libs/libopus_build_windows_x86_static_release.zip:
	@echo "	Fetching $@ from https://build.tox.chat"
	@cd libs/ && curl -s -O https://build.tox.chat/view/libopus/job/libopus_build_windows_x86_static_release/lastSuccessfulBuild/artifact/libopus_build_windows_x86_static_release.zip
	@unzip -qq -d libs/windows-x32/ $@

libs/libopenal_build_windows_x86_static_release.zip:
	@echo "	Fetching $@ from https://build.tox.chat"
	@cd libs/ && curl -s -O https://build.tox.chat/view/libopenal/job/libopenal_build_windows_x86_static_release/lastSuccessfulBuild/artifact/libopenal_build_windows_x86_static_release.zip
	@unzip -qq -d libs/windows-x32/ $@

libs/libtoxcore_build_windows_x86_static_release.zip:
	@echo "	Fetching $@ from https://build.tox.chat"
	@cd libs/ && curl -s -O https://build.tox.chat/view/libtoxcore/job/libtoxcore_build_windows_x86_static_release/lastSuccessfulBuild/artifact/libtoxcore_build_windows_x86_static_release.zip
	@unzip -qq -d libs/windows-x32/ $@

libs/libvpx_build_windows_x86_static_release.zip:
	@echo "	Fetching $@ from https://build.tox.chat"
	@cd libs/ && curl -s -O https://build.tox.chat/view/libvpx/job/libvpx_build_windows_x86_static_release/lastSuccessfulBuild/artifact/libvpx_build_windows_x86_static_release.zip
	@unzip -qq -d libs/windows-x32/ $@

libs/libsodium_build_windows_x86_static_release.zip:
	@echo "	Fetching $@ from https://build.tox.chat"
	@cd libs/ && curl -s -O https://build.tox.chat/view/libsodium/job/libsodium_build_windows_x86_static_release/lastSuccessfulBuild/artifact/libsodium_build_windows_x86_static_release.zip
	@unzip -qq -d libs/windows-x32/ $@

x64-libs : libs/libopus_build_windows_x86-64_static_release.zip libs/libopenal_build_windows_x86-64_static_release.zip libs/libtoxcore_build_windows_x86-64_static_release.zip libs/libvpx_build_windows_x86-64_static_release.zip libs/libsodium_build_windows_x86-64_static_release.zip libs/libfilteraudio-x86_64.zip
	@echo "Done!"

libs/libfilteraudio-x86_64.zip:
	@echo "	Fetching $@ from https://build.tox.chat"
	@cd libs/ && curl -s -O https://build.tox.chat/view/libfilteraudio/job/libfilteraudio_build_windows_x86-64_static_release/lastSuccessfulBuild/artifact/libfilteraudio.zip
	@mv ./libs/libfilteraudio.zip ./libs/libfilteraudio-x86_64.zip
	@unzip -qq -d libs/windows-x64/ $@

libs/libopus_build_windows_x86-64_static_release.zip:
	@echo "	Fetching $@ from https://build.tox.chat"
	@cd libs/ && curl -s -O https://build.tox.chat/view/libopus/job/libopus_build_windows_x86-64_static_release/lastSuccessfulBuild/artifact/libopus_build_windows_x86-64_static_release.zip
	@unzip -qq -d libs/windows-x64/ $@

libs/libopenal_build_windows_x86-64_static_release.zip:
	@echo "	Fetching $@ from https://build.tox.chat"
	@cd libs/ && curl -s -O https://build.tox.chat/view/libopenal/job/libopenal_build_windows_x86-64_static_release/lastSuccessfulBuild/artifact/libopenal_build_windows_x86-64_static_release.zip
	@unzip -qq -d libs/windows-x64/ $@

libs/libtoxcore_build_windows_x86-64_static_release.zip:
	@echo "	Fetching $@ from https://build.tox.chat"
	@cd libs/ && curl -s -O https://build.tox.chat/view/libtoxcore/job/libtoxcore_build_windows_x86-64_static_release/lastSuccessfulBuild/artifact/libtoxcore_build_windows_x86-64_static_release.zip
	@unzip -qq -d libs/windows-x64/ $@

libs/libvpx_build_windows_x86-64_static_release.zip:
	@echo "	Fetching $@ from https://build.tox.chat"
	@cd libs/ && curl -s -O https://build.tox.chat/view/libvpx/job/libvpx_build_windows_x86-64_static_release/lastSuccessfulBuild/artifact/libvpx_build_windows_x86-64_static_release.zip
	@unzip -qq -d libs/windows-x64/ $@

libs/libsodium_build_windows_x86-64_static_release.zip:
	@echo "	Fetching $@ from https://build.tox.chat"
	@cd libs/ && curl -s -O https://build.tox.chat/view/libsodium/job/libsodium_build_windows_x86-64_static_release/lastSuccessfulBuild/artifact/libsodium_build_windows_x86-64_static_release.zip
	@unzip -qq -d libs/windows-x64/ $@

dist: all-libs uTox-x64.exe uTox-x32.exe
	@echo "Going to build Win64 and Win32"
	@mkdir -p dist/
	@mv uTox-x64.exe dist/
	@mv uTox-x32.exe dist/

.PHONY: all clean dist libs
