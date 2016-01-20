.PHONY: all install

DBUS :=		1
FILTER_AUDIO :=	0
UNITY :=	0
V4LCONVERT :=	1

DEPS :=		openal

BIN_SUFFIX :=	.exe

OS_CFLAGS :=	-pthread -std=gnu99
OS_LDFLAGS :=	/usr/x86_64-w64-mingw32/sys-root/mingw/lib/libwinpthread.a \
		-liphlpapi -lws2_32 -lgdi32 -lmsimg32 -ldnsapi -lcomdlg32 \
		-Wl,-subsystem,windows -lwinmm -lole32 -loleaut32 -lstrmiids

DIRs :=		png windows

OBJs :=		utox-icon.o

include ${SRCDIR}/mk/common.mk

all: ${STATICBIN}

utox-icon.o: ${SRCDIR}/icons/icon.rc
	@cp $< utox-icon.rc
	@echo "  RES   $@"
	${SILENT}x86_64-w64-mingw32-windres icons/icon.rc -O coff -o utox-icon.o

install: ${STATICBIN}
	${INSTALL_PROGRAM_DIR} ${BINPREFIX}
	${INSTALL_PROGRAM} ${STATICBIN} ${BINPREFIX}/${BIN}

	${INSTALL_ICONS}

	${INSTALL_DATA_DIR} ${DATAPREFIX}/applications
	${INSTALL_DATA} ${SRCDIR}/src/utox.desktop ${DATAPREFIX}/applications/
	
	${INSTALL_MAN_DIR} ${MANPREFIX}/man1
	${INSTALL_MAN} ${SRCDIR}/src/utox.1 ${MANPREFIX}/man1
