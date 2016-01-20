#
# General
#

BIN :=		utox${BIN_SUFFIX}
STATICBIN :=	utox-static${BIN_SUFFIX}

CFLAGS ?=	-g -Wall -Wshadow  -fno-strict-aliasing
DEPS +=		libtoxcore libtoxav vpx libsodium

GIT_CMD :=	git describe --abbrev=8 --dirty --always --tags
GIT_V :=	$(shell cd ${SRCDIR} && ${GIT_CMD})

CFLAGS +=	${OS_CFLAGS} -DGIT_VERSION=\"${GIT_V}\"
LDFLAGS +=	${OS_LDFLAGS}

ifneq (${PSEUDOSTATIC},1)
SLDFLAGS +=	-static ${OS_LDFLAGS}
endif
SLDFLAGS +=	${OS_LDFLAGS}

define nl


endef

#
# Dependencies
#

PKG_CONFIG ?=   pkg-config

ifeq (${DBUS},1)
DEPS +=		dbus-1
else
CFLAGS +=	-DNO_DBUS
endif

ifeq (${FILTER_AUDIO},1)
DEPS +=		filteraudio
CFLAGS +=	-DAUDIO_FILTERING
endif

ifeq (${V4LCONVERT},1)
DEPS +=		libv4lconvert
else
CFLAGS +=	-DNO_V4LCONVERT
endif

ifeq (${UNITY},1)
DEPS +=		messaging-menu unity
CFLAGS +=	-DUNITY
endif

CFLAGS +=	$(shell ${PKG_CONFIG} --cflags ${DEPS})

LDFLAGS +=	$(shell ${PKG_CONFIG} --libs ${DEPS})

ifeq (${PSEUDOSTATIC},1)
LIBS :=		$(patsubst -l%,lib%.a,$(shell ${PKG_CONFIG} --libs-only-l ${DEPS}))
LDIRS :=	$(patsubst -L%,%,$(shell ${PKG_CONFIG} --libs-only-L ${DEPS}))
SLDFLAGS +=	$(wildcard $(foreach l,${LIBS},$(foreach d,${LDIRS},$d/$l)))
SLDFLAGS +=	$(shell ${PKG_CONFIG} --libs-only-other ${DEPS})
else
SLDFLAGS +=	$(shell ${PKG_CONFIG} --libs --static ${DEPS})
endif

#
# Sources
#

SRCs +=		$(wildcard ${SRCDIR}/src/*.c)
SRCs +=		$(foreach dir,${DIRs},$(wildcard ${SRCDIR}/src/${dir}/*.[cm]))

HEADERS +=		$(wildcard ${SRCDIR}/src/*.h ${SRCDIR}/src/*/*.h ${SRCDIR}/langs/*.h)

OBJs +=		$(patsubst %.m,%.o,$(patsubst %.c,%.o,${SRCs:${SRCDIR}/src/%=%}))

#
# Rules
#

ifndef V
SILENT :=	@
endif

VPATH =		${SRCDIR}/src

DESTDIR ?=
PREFIX ?= /usr/local
BINPREFIX ?= ${PREFIX}/bin
DATAPREFIX ?= ${PREFIX}/share
MANPREFIX ?= ${PREFIX}/share/man

SIZES =		14x14 16x16 22x22 24x24 32x32 48x48 64x64 72x72 96x96 128x128 \
		192x192 256x256 512x512
INSTALL_ICONS :=$(foreach size,${SIZES} scalable,${nl} \
	${INSTALL_DATA_DIR} ${DATAPREFIX}/icons/hicolor/${size}/apps)
INSTALL_ICONS +=${nl} \
	${INSTALL_DATA} ${SRCDIR}/icons/utox.svg \
			${DATAPREFIX}/icons/hicolor/scalable/apps

INSTALL_ICONS +=$(foreach size,${SIZES},${nl} \
	${INSTALL_DATA} ${SRCDIR}/icons/utox-${size}.png \
			${DATAPREFIX}/icons/hicolor/${size}/apps/utox.png)

%.o: %.c ${HEADERS}
	@mkdir -p $(dir $@)
	@echo "  CC    $@"
	${SILENT}${CC} ${CFLAGS} -o $@ -c $<

%.o : %.m ${HEADERS}
	@mkdir -p $(dir $@)
	@echo "  CC    $@"
	${SILENT}${CC} ${CFLAGS} -o $@ -c $<


${BIN}: ${OBJs}
	@echo "  LD    $@"
	${SILENT}${CC} -o $@ $^ ${LDFLAGS}

${STATICBIN}: ${OBJs}
	@echo "  SL    $@"
	${SILENT}${CC} -o $@ $^ ${SLDFLAGS}

info:
	@echo " CC            ${CC}"
	@echo " CFLAGS        ${CFLAGS}"
	@echo " LDFLAGS       ${LDFALGS}"
	@echo " Dependencies  ${DEPS}"
	@echo " OSTYPE        ${OSTYPE}"
	@echo " SRCDIR        ${SRCDIR}"
	@echo " SRCs          ${SRCs}"
	@echo " HEADERS       ${HEADERS}"
	@echo " OBJs          ${OBJs}"
