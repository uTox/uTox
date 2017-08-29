# - Try to find FilterAudio
# Once done this will define
#  LIBFILTERAUDIO_FOUND - System has FilterAudio
#  LIBFILTERAUDIO_INCLUDE_DIRS - The FilterAudio include directories
#  LIBFILTERAUDIO_LIBRARIES - The libraries needed to use FilterAudio
#  LIBFILTERAUDIO_DEFINITIONS - Compiler switches required for using FilterAudio

find_package(PkgConfig)

pkg_check_modules(PKG_LIBFILTERAUDIO QUIET libfilteraudio)
set(LIBFILTERAUDIO_DEFINITIONS ${PKG_LIBFILTERAUDIO_CFLAGS_OTHER})

find_path(LIBFILTERAUDIO_INCLUDE_DIR filter_audio.h HINTS
    ${PKG_LIBFILTERAUDIO_INCLUDEDIR}
    ${PKG_LIBFILTERAUDIO_INCLUDE_DIRS}
)

find_library(LIBFILTERAUDIO_LIBRARY NAMES filteraudio HINTS
    ${PKG_LIBFILTERAUDIO_LIBDIR}
    ${PKG_LIBFILTERAUDIO_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBFILTERAUDIO_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(
    libfilteraudio
    DEFAULT_MSG
    LIBFILTERAUDIO_LIBRARY
    LIBFILTERAUDIO_INCLUDE_DIR
)

mark_as_advanced(LIBFILTERAUDIO_INCLUDE_DIR LIBFILTERAUDIO_LIBRARY)

set(LIBFILTERAUDIO_LIBRARIES ${LIBFILTERAUDIO_LIBRARY})
set(LIBFILTERAUDIO_INCLUDE_DIRS ${LIBFILTERAUDIO_INCLUDE_DIR})
