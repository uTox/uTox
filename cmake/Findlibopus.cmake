# - Try to find opus
# Once done this will define
#  LIBOPUS_FOUND - System has opus
#  LIBOPUS_INCLUDE_DIRS - The opus include directories
#  LIBOPUS_LIBRARIES - The libraries needed to use opus
#  LIBOPUS_DEFINITIONS - Compiler switches required for using opus

find_package(PkgConfig)

pkg_check_modules(PKG_LIBOPUS QUIET libopus)
set(LIBOPUS_DEFINITIONS ${PKG_LIBOPUS_CFLAGS_OTHER})

find_path(LIBOPUS_INCLUDE_DIR opus/opus.h
    HINTS ${PKG_LIBOPUS_INCLUDEDIR} ${PKG_LIBOPUS_INCLUDE_DIRS}
)

find_library(LIBOPUS_LIBRARY NAMES opus
    HINTS ${PKG_LIBOPUS_LIBDIR} ${PKG_LIBOPUS_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBOPUS_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(
    libopus
    DEFAULT_MSG
    LIBOPUS_LIBRARY
    LIBOPUS_INCLUDE_DIR
)

mark_as_advanced(LIBOPUS_INCLUDE_DIR LIBOPUS_LIBRARY)

set(LIBOPUS_LIBRARIES ${LIBOPUS_LIBRARY})
set(LIBOPUS_INCLUDE_DIRS ${LIBOPUS_INCLUDE_DIR})
