# - Try to find FontConfig
# Once done this will define
#  LIBFONTCONFIG_FOUND - System has FontConfig
#  LIBFONTCONFIG_INCLUDE_DIRS - The FontConfig include directories
#  LIBFONTCONFIG_LIBRARIES - The libraries needed to use FontConfig
#  LIBFONTCONFIG_DEFINITIONS - Compiler switches required for using FontConfig

find_package(PkgConfig)

pkg_check_modules(PKG_LIBFONTCONFIG QUIET libfontconfig)
set(LIBFONTCONFIG_DEFINITIONS ${PKG_LIBFONTCONFIG_CFLAGS_OTHER})

find_path(LIBFONTCONFIG_INCLUDE_DIR fontconfig/fontconfig.h HINTS
    ${PKG_LIBFONTCONFIG_INCLUDEDIR}
    ${PKG_LIBFONTCONFIG_INCLUDE_DIRS}
)

find_library(LIBFONTCONFIG_LIBRARY NAMES fontconfig HINTS
    ${PKG_LIBFONTCONFIG_LIBDIR}
    ${PKG_LIBFONTCONFIG_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set LIBFONTCONFIG_FOUND to TRUE
# if all listed variables are TRUE.
find_package_handle_standard_args(
    libfontconfig
    DEFAULT_MSG
    LIBFONTCONFIG_LIBRARY
    LIBFONTCONFIG_INCLUDE_DIR
)

mark_as_advanced(LIBFONTCONFIG_INCLUDE_DIR LIBFONTCONFIG_LIBRARY)

set(LIBFONTCONFIG_LIBRARIES ${LIBFONTCONFIG_LIBRARY})
set(LIBFONTCONFIG_INCLUDE_DIRS ${LIBFONTCONFIG_INCLUDE_DIR})
