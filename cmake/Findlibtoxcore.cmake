# - Try to find ToxCore
# Once done this will define
#  LIBTOXCORE_FOUND - System has ToxCore
#  LIBTOXCORE_INCLUDE_DIRS - The ToxCore include directories
#  LIBTOXCORE_LIBRARIES - The libraries needed to use ToxCore
#  LIBTOXCORE_DEFINITIONS - Compiler switches required for using ToxCore

find_package(PkgConfig)

pkg_check_modules(PKG_LIBTOXCORE QUIET libtoxcore)
set(LIBTOXCORE_DEFINITIONS ${PKG_LIBTOXCORE_CFLAGS_OTHER})

find_path(LIBTOXCORE_INCLUDE_DIR tox/tox.h
    HINTS ${PKG_LIBTOXCORE_INCLUDEDIR} ${PKG_LIBTOXCORE_INCLUDE_DIRS}
)

find_library(LIBTOXCORE_LIBRARY NAMES toxcore
    HINTS ${PKG_LIBTOXCORE_LIBDIR} ${PKG_LIBTOXCORE_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBTOXCORE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(
    libtoxcore
    DEFAULT_MSG
    LIBTOXCORE_LIBRARY
    LIBTOXCORE_INCLUDE_DIR
)

mark_as_advanced(LIBTOXCORE_INCLUDE_DIR LIBTOXCORE_LIBRARY)

set(LIBTOXCORE_LIBRARIES ${LIBTOXCORE_LIBRARY})
set(LIBTOXCORE_INCLUDE_DIRS ${LIBTOXCORE_INCLUDE_DIR})
