.PHONY: clean info install

## Configuration

# General settings

PREFIX ?=		/usr/local
BINDIR ?=		${PREFIX}/bin
DATADIR ?=		${PREFIX}/share/utox
MANDIR ?=		${PREFIX}/share/man

INSTALL_PROGRAM =	install -c -m 755
INSTALL_PROGRAM_DIR =	install -d -m 755
INSTALL_MAN =		install -c  -m 644
INSTALL_MAN_DIR =	install -d -m 755
INSTALL_DATA =		install -c  -m 644
INSTALL_DATA_DIR =	install -d -m 755


SUBDIRs =		src src/png src/xlib
SRCs ?!=		find ${SUBDIRs} -maxdepth 1 -type f -name *.c
OBJs ?=			${SRCs:src/%.c=%.o} utox-128x128.o

ARCH !=			uname -m
BUILDDIR ?=		build-${ARCH}
BIN =			${BUILDDIR}/utox

SIZES =			14x14 16x16 22x22 24x24 32x32 36x36 48x48 64x64 72x72 \
			96x96 128x128 192x192 256x256 512x512

CFLAGS ?=		-g -Wall -Wextra
CFLAGS +=		-std=gnu99
LDFLAGS +=		-pthread -lm

GIT_V !=		git describe --abbrev=8 --dirty --always --tags
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

AUTO ?= yes
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

${BIN}:
	@mkdir -p ${SUBDIRs:src%=${BUILDDIR}%}
	@cd ${BUILDDIR} && ${MAKE} -f ${.CURDIR}/makefile CONFIGURE=yes \
		SRCs="${SRCs}" OBJs="${OBJs}" SRCPREFIX="${.CURDIR}" utox

.for src in ${SRCs}
${src:src/%.c=%.o}: ${SRCPREFIX}/${src}
	${CC} ${CFLAGS} -c -o $@ ${SRCPREFIX}/${src}
.endfor

.MADE: icons/utox-128x128.png

icons/utox-128x128.png:
	@mkdir icons
	@ln ${SRCPREFIX}/$@ $@ 2> /dev/null || cp ${SRCPREFIX}/$@ $@

utox-128x128.o: icons/utox-128x128.png
	${LD} -r -b binary -o $@ icons/utox-128x128.png

utox: ${HDRs} ${OBJs}
	${CC} -o $@ ${LDFLAGS} ${OBJs}

## Install rules

install: ${BIN}
	${INSTALL_PROGRAM_DIR} ${BINDIR}
	${INSTALL_PROGRAM} utox ${BINDIR}/utox
	${INSTALL_DATA_DIR} ${DATADIR}
.for size in ${SIZES}
	${INSTALL_DATA_DIR} ${DATADIR}/icons/hicolor/${size}/apps
	${INSTALL_DATA} icons/utox-${size}.png \
	                ${DATADIR}/icons/hicolor/${size}/apps/utox-${size}.png
.endfor
	${INSTALL_DATA_DIR} ${DATADIR}/icons/hicolor/scalable/apps
	${INSTALL_DATA} icons/utox.svg \
	                ${DATADIR}/icons/hicolor/scalable/apps/utox.svg
	${INSTALL_DATA_DIR} ${DATADIR}/applications
	${INSTALL_DATA} src/utox.desktop ${DATADIR}/applications/utox.desktop
.if !empty(${OPTDEPs:Munity})
	echo "X-MessagingMenu-UsesChatSection=true" >> \
	     ${DATADIR}/applications/utox.desktop
.endif
	${INSTALL_MAN_DIR} ${MANDIR}/man1
	${INSTALL_MAN} src/utox.1 ${MANDIR}/man1/utox.1

clean:
	rm -rf ${BUILDDIR}

info:
	@echo "CC:            ${CC}"
	@echo "CFLAGS:        ${CFLAGS}"
	@echo "LDFLAGS:       ${LDFLAGS}"
	@echo "dependencies:  ${DEPs}"
	@echo "~ optional:    ${OPTDEPs} ${AUTO:L:S/yes/(guessed)/}"
	@echo "headers:       ${HDRs}"
	@echo "sources:       ${SRCs}"
	@echo "objects:       ${OBJs}"
