.PHONY: all install

DBUS :=		0
FILTER_AUDIO :=	0
UNITY :=	0
V4LCONVERT :=	0

DEPS :=		openal

BIN_SUFFIX :=	.exe

OS_CFLAGS :=	-pthread -std=gnu99
OS_LDFLAGS :=	/usr/x86_64-w64-mingw32/sys-root/mingw/lib/libwinpthread.a \
		-liphlpapi -lws2_32 -lgdi32 -lmsimg32 -ldnsapi -lcomdlg32 \
		-Wl,-subsystem,windows -lwinmm -lole32 -loleaut32 -lstrmiids

DIRs :=		png windows

OBJs :=		utox-icon.o

TOOL_PREFIX ?=	x86_64-w64-mingw32-
PKG_CONFIG ?=	${TOOL_PREFIX}pkg-config

include ${SRCDIR}/mk/common.mk

all: ${STATICBIN}

utox-icon.o: ${SRCDIR}/icons/icon.rc
	@echo "  RES   $@"
	${SILENT}${TOOL_PREFIX}windres -O coff -i $< -o $@
