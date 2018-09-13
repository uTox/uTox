# - Try to find V4Lconvert
# Once done this will define
#  LIBV4LCONVERT_FOUND - System has V4Lconvert
#  LIBV4LCONVERT_INCLUDE_DIRS - The V4Lconvert include directories
#  LIBV4LCONVERT_LIBRARIES - The libraries needed to use V4Lconvert
#  LIBV4LCONVERT_DEFINITIONS - Compiler switches required for using V4Lconvert

find_package(PkgConfig)

pkg_check_modules(PKG_LIBV4LCONVERT QUIET libv4lconvert)
set(LIBV4LCONVERT_DEFINITIONS ${PKG_LIBV4LCONVERT_CFLAGS_OTHER})

find_path(LIBV4LCONVERT_INCLUDE_DIR libv4lconvert.h HINTS
    ${PKG_LIBV4LCONVERT_INCLUDEDIR}
    ${PKG_LIBV4LCONVERT_INCLUDE_DIRS}
)

find_library(LIBV4LCONVERT_LIBRARY NAMES v4lconvert HINTS
    ${PKG_LIBV4LCONVERT_LIBDIR}
    ${PKG_LIBV4LCONVERT_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set LIBV4LCONVERT_FOUND to TRUE
# if all listed variables are TRUE.
find_package_handle_standard_args(
    libv4lconvert
    DEFAULT_MSG
    LIBV4LCONVERT_LIBRARY
    LIBV4LCONVERT_INCLUDE_DIR
)

mark_as_advanced(LIBV4LCONVERT_INCLUDE_DIR LIBV4LCONVERT_LIBRARY)

set(LIBV4LCONVERT_LIBRARIES ${LIBV4LCONVERT_LIBRARY})
set(LIBV4LCONVERT_INCLUDE_DIRS ${LIBV4LCONVERT_INCLUDE_DIR})
