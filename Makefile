# This Makefile completes the first stage of uTox build, which consists of:
#
#   * detects operating system
#   * detects path to the uTox distribution
#   * determines build directory
#   * creates build directory
#   * calls relevant makefile from the build directory
#

OS :=		$(shell uname -s)

# Cygwin and MinGW put too much details into "uname -s".  Normalize to "Windows".
ifneq ($(filter CYGWIN%, ${OS}),)
OS :=		Windows
endif
ifneq ($(filter MINGW%, ${OS}),)
OS :=		Windows
endif

ifeq (${OS}, Windows)
OSTYPE = 	windows
else

ifeq (${OS}, Darwin)
OSTYPE = osx
else
OSTYPE = unix
endif

endif

SRCDIR :=	$(realpath $(dir $(lastword ${MAKEFILE_LIST})))

ifndef BUILDDIR
ifeq (${SRCDIR},${CURDIR})
BUILDDIR :=	build-${OS}-$(shell uname -m)
endif
endif

export SRCDIR OS OSTYPE
export CC CFLAGS LD LDFLAGS DEPS
export DBUS FILTER_AUDIO UNITY V4LCONVERT

GOALS := $(filter-out all build info clean,${MAKECMDGOALS})

.PHONY: all build info clean ${GOALS}

all: build

${BUILDDIR}:
	@mkdir "${BUILDDIR}"

build: ${BUILDDIR}
	@cd "${BUILDDIR}" && ${MAKE} -f ${SRCDIR}/mk/${OSTYPE}.mk all

info:
	@${MAKE} -f ${SRCDIR}/mk/${OSTYPE}.mk info

ifneq ($(realpath $(dir ${BUILDDIR}/x)), ${CURDIR})
clean:
	@rm -Rf "${BUILDDIR}"
endif

${GOALS}: ${BUILDDIR}
	cd "${BUILDDIR}" && ${MAKE} -f ${SRCDIR}/mk/${OSTYPE}.mk $@
