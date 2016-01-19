.PHONY: all install

DBUS :=		1
FILTER_AUDIO :=	0
UNITY :=	0
V4LCONVERT :=	1

DEPS :=		openal fontconfig freetype2 x11 xext xrender

OS_CFLAGS :=	-pthread -std=gnu99
OS_LDFLAGS :=	-pthread -lm

ifeq (${OS},Linux)
OS_CFLAGS +=	-DLINUX_IO
OS_LDFLAGS +=	-ldl
endif

ifneq (${OS},OpenBSD)
OS_LDFLAGS +=	-lresolv
endif

DIRs :=		png xlib

OBJs :=		utox-icon.o

include ${SRCDIR}/mk/common.mk

all: ${BIN}

utox-icon.o: ${SRCDIR}/icons/utox-128x128.png
	@cp $< utox-icon.png
	@echo "  LD    $@"
	${SILENT}${LD} -r -b binary utox-icon.png -o utox-icon.o

install: ${BIN}
	${INSTALL_PROGRAM_DIR} ${BINPREFIX}
	${INSTALL_PROGRAM} ${BIN} ${BINPREFIX}

	${INSTALL_ICONS}

	${INSTALL_DATA_DIR} ${DATAPREFIX}/applications
	${INSTALL_DATA} ${SRCDIR}/src/utox.desktop ${DATAPREFIX}/applications/
	
	${INSTALL_MAN_DIR} ${MANPREFIX}/man1
	${INSTALL_MAN} ${SRCDIR}/src/utox.1 ${MANPREFIX}/man1
