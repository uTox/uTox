#Dumb makefile by Dude
#Change to i686-w64-mingw32 in case 32bit
WINDOWS_TOOLCHAIN=x86_64-w64-mingw32
CFLAGS=-s -Ofast -std=gnu99
CFLAGS+= -DAUDIO_FILTERING -DAL_LIBTYPE_STATIC -I ../lib/filter_audio/ -I ../lib/toxcore/include/ -I ../lib/openal/include/
LDFLAGS= ../lib/openal/lib/libOpenAL32.a ../lib/toxcore/lib/libtoxav.a ../lib/toxcore/lib/libtoxdns.a ../lib/toxcore/lib/libtoxcore.a ../lib/toxcore/lib/libvpx.a ../lib/toxcore/lib/libopus.a ../lib/toxcore/lib/libsodium.a $(MINGW32_LIB_DIR)/libwinpthread.a -liphlpapi -lws2_32 -lgdi32 -lmsimg32 -ldnsapi -lcomdlg32 -Wl,-subsystem,windows -lwinmm -lole32 -loleaut32 -lstrmiids

MINGW32_LIB_DIR=/usr/$(WINDOWS_TOOLCHAIN)/lib

src=$(wildcard ../*.c ../lib/filter_audio/filter_audio.c ../lib/filter_audio/aec/*.c ../lib/filter_audio/agc/*.c ../lib/filter_audio/ns/*.c ../lib/filter_audio/other/*.c ../lib/filter_audio/zam/*.c ../png/png.c )
_objs = $(src:.c=.o)
objs = $(patsubst %,obj/trap/%,$(_objs))

all: init icon utox.exe

test:
	echo $(objs)

init:
	mkdir -p obj/trap
	mkdir -p obj/png
	mkdir -p obj/lib/filter_audio
	mkdir -p obj/lib/filter_audio/aec
	mkdir -p obj/lib/filter_audio/agc
	mkdir -p obj/lib/filter_audio/ns
	mkdir -p obj/lib/filter_audio/other
	mkdir -p obj/lib/filter_audio/zam
	mkdir -p result
icon:
	$(WINDOWS_TOOLCHAIN)-windres ../icons/icon.rc -O coff -o obj/icon.o

utox.exe: $(objs)
	@echo "  LINK   $@"
	@$(WINDOWS_TOOLCHAIN)-gcc $(CFLAGS) -o result/utox.exe $(objs) $(LDFLAGS)

obj/trap/%.o: %.c
	@echo " MINGW  $@"
	@$(WINDOWS_TOOLCHAIN)-gcc $(CFLAGS) -o $@ -c $<

clean:
	rm -rf obj/*
	rm -rf result/*
