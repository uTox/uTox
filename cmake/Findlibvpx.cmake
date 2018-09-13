# - Try to find VPX
# Once done this will define
#  LIBVPX_FOUND - System has VPX
#  LIBVPX_INCLUDE_DIRS - The VPX include directories
#  LIBVPX_LIBRARIES - The libraries needed to use VPX
#  LIBVPX_DEFINITIONS - Compiler switches required for using VPX

find_package(PkgConfig)

pkg_check_modules(PKG_LIBVPX QUIET libvpx)
set(LIBVPX_DEFINITIONS ${PKG_LIBVPX_CFLAGS_OTHER})

find_path(LIBVPX_INCLUDE_DIR vpx/vpx_codec.h HINTS
    ${PKG_LIBVPX_INCLUDEDIR}
    ${PKG_LIBVPX_INCLUDE_DIRS}
)

find_library(LIBVPX_LIBRARY NAMES vpx HINTS
    ${PKG_LIBVPX_LIBDIR}
    ${PKG_LIBVPX_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set LIBVPX_FOUND to TRUE
# if all listed variables are TRUE.
find_package_handle_standard_args(
    libvpx
    DEFAULT_MSG
    LIBVPX_LIBRARY
    LIBVPX_INCLUDE_DIR
)

mark_as_advanced(LIBVPX_INCLUDE_DIR LIBVPX_LIBRARY)

set(LIBVPX_LIBRARIES ${LIBVPX_LIBRARY})
set(LIBVPX_INCLUDE_DIRS ${LIBVPX_INCLUDE_DIR})
