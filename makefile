.PHONY: all clean info install

## Configuration

# Separate build

.for mkfile in ${MAKEFILE_LIST}
THIS_MAKEFILE = ${mkfile}
.endfor
DIST = ${THIS_MAKEFILE:makefile=}

# General settings

PREFIX ?=		/usr/local
BINDIR ?=		${PREFIX}/bin
DATADIR ?=		${PREFIX}/share
MANDIR ?=		${PREFIX}/share/man

INSTALL_DATA =		install -c -m 644
INSTALL_DATA_DIR =	install -d -m 755
INSTALL_MAN =		install -c -m 644
INSTALL_MAN_DIR =	install -d -m 755
INSTALL_PROGRAM =	install -c -m 755
INSTALL_PROGRAM_DIR =	install -d -m 755

SUBDIRs =		png xlib
SRCDIRs =		${DIST}src ${SUBDIRs:%=${DIST:Q}src/%}
SRCs !=			find ${SRCDIRs} -maxdepth 1 -type f -name *.c
RELSRCs =		${SRCs:S/${DIST:Q}src\///}
OBJs =			${RELSRCs:.c=.o} utox-128x128.o

ARCH !=			uname -m
.if empty(DIST)
BUILDDIR ?=		build-${ARCH}/
.endif
BIN =			${BUILDDIR}utox

SIZES =			14x14 16x16 22x22 24x24 32x32 36x36 48x48 64x64 72x72 \
			96x96 128x128 192x192 256x256 512x512

CFLAGS ?=		-g -Wall -Wextra
CFLAGS +=		-std=gnu99
LDFLAGS +=		-pthread -lm

GIT_V !=		git describe --abbrev=8 --dirty --always --tags 2> \
			/dev/null || echo unknown
CFLAGS +=		-DGIT_VERSION=\"${GIT_V}\"

# OS-dependent linker flags

OS !=			uname -s

.if !empty(OS:MLinux)
CFLAGS +=		-DLINUX_IO
LDFLAGS +=		-ldl
.endif

.if empty(OS:MOpenBSD)
LDFLAGS +=		-lresolv
.endif

# Dependency management

DEPs =			libtoxav libtoxcore openal vpx libsodium fontconfig \
			freetype2 x11 xext xrender

AUTO ?= Yes
.if ${AUTO:L} == yes
OPTDEPs +!=		pkg-config dbus-1 && echo dbus-1 || echo
OPTDEPs +!=		pkg-config filteraudio && echo filteraudio || echo
OPTDEPs +!=		pkg-config libv4lconvert && echo libv4lconvert || echo
OPTDEPs +!=		pkg-config unity && echo unity || echo
.endif

.if empty(OPTDEPs:Mdbus-1)
CFLAGS +=		-DNO_DBUS
.endif

.if !empty(OPTDEPs:Mfilteraudio)
CFLAGS +=		-DAUDIO_FILTERING
.endif

.if empty(OPTDEPs:Mlibv4lconvert)
CFLAGS +=		-DNO_V4LCONVERT
.endif

.if !empty(OPTDEPs:Munity)
CFLAGS +=		-DUNITY
OPTDEPs +=		messaging-menu
.endif

CFLAGS +!=		pkg-config --cflags ${DEPs} ${OPTDEPs}
LDFLAGS +!=		pkg-config --libs ${DEPs} ${OPTDEPs}

## Build rules

all: ${BIN}

${SUBDIRs}:
	@mkdir -p $@

.for src in ${RELSRCs}
${src:.c=.o}: ${DIST}src/${src}
	${CC} ${CFLAGS} -c -o $@ ${DIST}src/${src}
.endfor

icons/utox-128x128.png:
	@mkdir icons
	@ln ${DIST}$@ $@ 2> /dev/null || cp ${DIST}$@ $@

utox-128x128.o: icons/utox-128x128.png
	${LD} -r -b binary -o $@ icons/utox-128x128.png

utox: ${SUBDIRs} ${OBJs}
	${CC} -o $@ ${LDFLAGS} ${OBJs}

${BIN}: ${SRCs}
	@mkdir -p ${BUILDDIR}
	cd ${BUILDDIR}; ${MAKE} -f ${.CURDIR}/makefile

## Install rules

install: ${BIN}
	${INSTALL_PROGRAM_DIR} ${BINDIR}
	${INSTALL_PROGRAM} ${BUILDDIR}utox ${BINDIR}/utox
	${INSTALL_DATA_DIR} ${DATADIR}
.for size in ${SIZES}
	${INSTALL_DATA_DIR} ${DATADIR}/icons/hicolor/${size}/apps
	${INSTALL_DATA} ${DIST}icons/utox-${size}.png \
	                ${DATADIR}/icons/hicolor/${size}/apps/utox.png
.endfor
	${INSTALL_DATA_DIR} ${DATADIR}/icons/hicolor/scalable/apps
	${INSTALL_DATA} ${DIST}icons/utox.svg \
	                ${DATADIR}/icons/hicolor/scalable/apps/utox.svg
	${INSTALL_DATA_DIR} ${DATADIR}/applications
	${INSTALL_DATA} ${DIST}src/utox.desktop \
			${DATADIR}/applications/utox.desktop
.if !empty(${OPTDEPs:Munity})
	echo "X-MessagingMenu-UsesChatSection=true" >> \
	     ${DATADIR}/applications/utox.desktop
.endif
	${INSTALL_MAN_DIR} ${MANDIR}/man1
	${INSTALL_MAN} ${DIST}src/utox.1 ${MANDIR}/man1/utox.1

clean:
	@rm -rf ${BUILDDIR}

info:
	@echo "CC:            ${CC}"
	@echo "CFLAGS:        ${CFLAGS}"
	@echo "LDFLAGS:       ${LDFLAGS}"
	@echo "dependencies:  ${DEPs}"
.if ${AUTO:L} == yes
	@echo "~ optional:    ${OPTDEPs} (autodetected)"
.else
	@echo "~ optional:    ${OPTDEPs}"
.endif
	@echo "sources:       ${SRCs}"
	@echo "objects:       ${OBJs}"
