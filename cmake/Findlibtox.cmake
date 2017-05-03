# - Try to find Tox
# Once done this will define
#  LIBTOX_FOUND - System has Tox
#  LIBTOX_INCLUDE_DIRS - The Tox include directories
#  LIBTOX_LIBRARY_DIRS - The Tox lib directories
#  LIBTOX_LIBRARIES - The libraries needed to use Tox
#  LIBTOX_DEFINITIONS - Compiler switches required for using Tox
#
# COMPONENTS
#   toxencryptsave
#   toxav
#   toxdns
#   toxcore
#   toxgroup
#   toxmessenger
#   toxfriends
#   toxdht
#   toxnetcrypto
#   toxcrypto
#   toxnetwork

find_package(PkgConfig)

if(libtox_FIND_COMPONENTS)
    set(_TOX_COMPNENTS ${libtox_FIND_COMPONENTS})
else()
    set(_TOX_COMPNENTS toxencryptsave toxdns toxav toxcore) # default components
endif()

pkg_check_modules(_PKG_TOX QUIET libtoxcore)
set(LIBTOX_DEFINITIONS ${_PKG_TOX_CFLAGS_OTHER})

find_path(LIBTOX_INCLUDE_DIR tox/tox.h
    HINTS ${_PKG_TOX_INCLUDEDIR} ${_PKG_TOX_INCLUDE_DIRS}
)

# required components
foreach(_COMPNENT ${_TOX_COMPNENTS})
    find_library(_TEMP NAMES ${_COMPNENT}
        HINTS ${_PKG_TOX_LIBDIR} ${_PKG_TOX_LIBRARY_DIRS}
    )
    list(APPEND LIBTOX_LIBRARIES ${_TEMP})
    unset(_TEMP CACHE)
endforeach()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBTOX_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(
    libtox
    DEFAULT_MSG
    LIBTOX_LIBRARIES
    LIBTOX_INCLUDE_DIR
)

mark_as_advanced(_TOX_COMPNENTS _PKG_TOX LIBTOX_INCLUDE_DIR)

set(LIBTOX_INCLUDE_DIRS ${LIBTOX_INCLUDE_DIR})
set(LIBTOX_LIBRARY_DIRS ${_PKG_TOX_LIBRARY_DIRS})
set(LIBTOX_LIBRARY_DIR ${_PKG_TOX_LIBDIR})

unset(_TOX_COMPNENTS)
unset(_COMPNENT)
unset(_PKG_TOX)
unset(_TEMP)
