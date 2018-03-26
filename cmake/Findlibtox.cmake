# - Try to find Tox
# Once done this will define
#  LIBTOX_FOUND - System has Tox
#  LIBTOX_INCLUDE_DIRS - The Tox include directories
#  LIBTOX_LIBRARIES - The libraries needed to use Tox
#  LIBTOX_DEFINITIONS - Compiler switches required for using Tox

find_package(PkgConfig)

pkg_check_modules(PKG_LIBTOX QUIET libtoxcore)
set(LIBTOX_DEFINITIONS ${PKG_LIBTOX_CFLAGS_OTHER})

find_path(LIBTOX_INCLUDE_DIR tox/tox.h HINTS
    ${PKG_LIBTOX_INCLUDEDIR}
    ${PKG_LIBTOX_INCLUDE_DIRS}
)

find_library(LIBTOX_LIBRARY NAMES toxcore HINTS
    ${PKG_LIBTOX_LIBDIR}
    ${PKG_LIBTOX_LIBRARY_DIRS}
)
find_library(LIBTOXAV_LIBRARY NAMES toxav HINTS
    ${PKG_LIBTOXAV_LIBDIR}
    ${PKG_LIBTOXAV_LIBRARY_DIRS}
)
find_library(LIBTOXENCSAVE_LIBRARY NAMES toxencryptsave HINTS
    ${PKG_LIBTOXENCSAVE_LIBDIR}
    ${PKG_LIBTOXENCSAVE_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set LIBTOX_FOUND to TRUE
# if all listed variables are TRUE.
find_package_handle_standard_args(
    libtox
    DEFAULT_MSG
    LIBTOX_LIBRARY
    LIBTOXAV_LIBRARY
    LIBTOXENCSAVE_LIBRARY
    LIBTOX_INCLUDE_DIR
)

mark_as_advanced(LIBTOX_INCLUDE_DIR LIBTOX_LIBRARY)

set(LIBTOX_LIBRARIES ${LIBTOX_LIBRARY} ${LIBTOXAV_LIBRARY} ${LIBTOXENCSAVE_LIBRARY})
set(LIBTOX_INCLUDE_DIRS ${LIBTOX_INCLUDE_DIR})
