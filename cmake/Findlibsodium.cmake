# - Try to find sodium
# Once done this will define
#  LIBSODIUM_FOUND - System has sodium
#  LIBSODIUM_INCLUDE_DIRS - The sodium include directories
#  LIBSODIUM_LIBRARIES - The libraries needed to use sodium
#  LIBSODIUM_DEFINITIONS - Compiler switches required for using sodium

find_package(PkgConfig)

pkg_check_modules(PKG_LIBSODIUM QUIET libsodium)
set(LIBSODIUM_DEFINITIONS ${PKG_LIBSODIUM_CFLAGS_OTHER})

find_path(LIBSODIUM_INCLUDE_DIR sodium.h
    HINTS ${PKG_LIBSODIUM_INCLUDEDIR} ${PKG_LIBSODIUM_INCLUDE_DIRS}
)

find_library(LIBSODIUM_LIBRARY NAMES sodium
    HINTS ${PKG_LIBSODIUM_LIBDIR} ${PKG_LIBSODIUM_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBSODIUM_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(
    libsodium
    DEFAULT_MSG
    LIBSODIUM_LIBRARY
    LIBSODIUM_INCLUDE_DIR
)

mark_as_advanced(LIBSODIUM_INCLUDE_DIR LIBSODIUM_LIBRARY)

set(LIBSODIUM_LIBRARIES ${LIBSODIUM_LIBRARY})
set(LIBSODIUM_INCLUDE_DIRS ${LIBSODIUM_INCLUDE_DIR})
