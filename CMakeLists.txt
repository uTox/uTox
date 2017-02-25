cmake_minimum_required(VERSION 3.2)
project(uTox LANGUAGES C)

#######################################################################
#
# project version
#
#######################################################################

# This version is for the entire project. All libraries (core, av, ...) move in
# versions in a synchronised way.
set(PROJECT_VERSION_MAJOR "0")
set(PROJECT_VERSION_MINOR "13")
set(PROJECT_VERSION_PATCH "0")
set(PROJECT_VERSION
  "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
# make version available in C files
configure_file(${uTox_SOURCE_DIR}/src/branding.h.in
               ${uTox_SOURCE_DIR}/src/branding.h)
configure_file(${uTox_SOURCE_DIR}/src/cocoa/Info.plist.in
               ${uTox_SOURCE_DIR}/src/cocoa/Info.plist)
configure_file(${uTox_SOURCE_DIR}/src/android/AndroidManifest.xml.in
               ${uTox_SOURCE_DIR}/src/android/AndroidManifest.xml)

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
    execute_process(COMMAND git describe --abbrev=8 --dirty --always --tags
                    OUTPUT_VARIABLE GIT_VERSION)
    string(REPLACE "\n" " " GIT_VERSION ${GIT_VERSION})
endif()

#######################################################################
#
# C compiler flags
#
#######################################################################

set(CMAKE_C_STANDARD 11) # this requires at least cmake 3.1

include(CheckCCompilerFlag)

# add compiler flag for all build types
function(add_cflag flag)
    string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" var ${flag})
    if(NOT DEFINED HAVE_C${var})
        message(STATUS "checking for C compiler flag: ${flag}")
    endif()
    set(CMAKE_REQUIRED_QUIET TRUE)

    check_c_compiler_flag("${flag}" HAVE_C${var} QUIET)
    if(HAVE_C${var})
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" PARENT_SCOPE)
    endif()
endfunction()

# Grab environment CFLAGS.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} $ENV{CFLAGS}")

# Warn on non-ISO C.
#add_cflag("-pedantic") # throws a lot of warning, quite sure we do not want this at current state.

# Add all warning flags we can.
add_cflag("-Wall")
add_cflag("-Wextra")

# Warnings we don't tolerate
add_cflag("-Werror=implicit-function-declaration")

# Warnings we don't care about
add_cflag("-Wformat=0")

# Everything Else
add_cflag("-fno-strict-aliasing")

# Have ld strip the symbols from Release and MinSizeRel build types.
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -s")

# Fix GNU stack
IF(CMAKE_C_COMPILER_ID MATCHES "GNU")
    IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,noexecstack")
    endif()
endif()

# set define for GIT_VERSION
if(GIT_VERSION)
    add_cflag("-DGIT_VERSION='\"${GIT_VERSION}\"'")
endif()

#######################################################################
#
# Build options
#
#######################################################################
# sanitize memory issues in Debug builds
# https://gcc.gnu.org/gcc-4.8/changes.html
# https://github.com/google/sanitizers/wiki/AddressSanitizer
option(ENABLE_ASAN "Enable Address Sanitizer on debug builds" ON)
if (ENABLE_ASAN)
    message("")
    message("  TIP: You may want to set the env variable  ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer  for ASAN to show nicer stack traces.")
    message("       See <http://clang.llvm.org/docs/AddressSanitizer.html#symbolizing-the-reports> for more details.")
    message("")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")
endif()

option(FILTER_AUDIO "Enable Filter Audio" ON)
if(FILTER_AUDIO)
    add_cflag("-DAUDIO_FILTERING")
endif()

option(UTOX_STATIC "Link uTox statically" OFF)
if(UTOX_STATIC)
    add_cflag("-static")
endif()

option(ENABLE_UPDATER "Enable updater" ON)
if(NOT UNIX AND ENABLE_UPDATER)
    add_cflag("-DENABLE_UPDATER")
endif()

option(TOXCORE_STATIC "Build uTox with the static version of Toxcore" OFF)
if(TOXCORE_STATIC)
    # The static windows libs need all of these... because REASONS!
    set(TOX_LIBS
        toxencryptsave
        toxav
        toxdns
        toxcore
        toxgroup
        toxmessenger
        toxfriends
        toxdht
        toxnetcrypto
        toxcrypto
        toxnetwork
        )
else()
    set(TOX_LIBS
        toxencryptsave
        toxdns
        toxav
        toxcore
        )
endif()

# print a summary of build options
message("Build options:")
message("- Add ASAN:        ${ENABLE_ASAN}")
message("- Filter Audio:    ${FILTER_AUDIO}")
message("- uTox Static:     ${UTOX_STATIC}")
message("- Toxcore Static:  ${TOXCORE_STATIC}")
message("- Windows Legacy:  ${WIN_XP_LEGACY}")

message("* C Compiler is '${CMAKE_C_COMPILER}' with the following flags:")
message("* C flags for Debug:       ${CMAKE_C_FLAGS_DEBUG}")
message("* C flags for Release:     ${CMAKE_C_FLAGS_RELEASE}")
message("* C flags for all types:   ${CMAKE_C_FLAGS}")


# Protip, you must use a different directory for each build target...
# -DMAKE_TOOLCHAIN_FILE has no effect unless the target directory is empty
# 1.5 hours to learn this...
if(WIN32)
    if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        set(GUI_TYPE WIN32)
    endif()
    ############################################################
    ## This is a hack, that we can remove once we have better
    ## separation of the sections of uTox, until then, this
    ## makes everything work.
    ############################################################
    include_directories( libs/toktok/include libs/windows-x64/include/ )
    if(WIN64)
        link_directories( libs/toktok/lib /usr/x86_64-w64-mingw32/lib/ libs/windows-x64/lib )
    else()
        link_directories( libs/toktok_32/lib /usr/i686-w64-mingw32/lib/ libs/windows-x32/lib )
    endif()

    ## Needed to build the widows icon
    enable_language(RC)
    set(WINDOWS_ICON icons/icon.rc)

    add_subdirectory(src/windows)
elseif(APPLE)
    set(GUI_TYPE MACOSX_BUNDLE)
    set(MACOSX_BUNDLE_INFO_PLIST src/cocoa/Info.plist)
    add_definitions("-x objective-c")
    add_subdirectory(src/cocoa)
    set(LIBRARIES "opus")
elseif(UNIX)
    add_subdirectory(src/xlib)
endif()

# include utoxUI
add_subdirectory(src/ui)
# include utoxAV
add_subdirectory(src/av)

#######################################################################
#
# :: uTox main
#
#######################################################################
add_executable(utox ${GUI_TYPE}
    src/avatar.c
    src/chatlog.c
    src/command_funcs.c
    src/commands.c
    src/devices.c
    src/dns.c
    src/file_transfers.c
    src/filesys.c
    src/flist.c
    src/friend.c
    src/groups.c
    src/inline_video.c
    src/logging.c
    src/main.c
    src/messages.c
    src/notify.c
    src/screen_grab.c
    src/self.c
    src/settings.c
    src/sized_string.h
    src/stb_image.h
    src/stb_image_write.h
    src/text.c
    src/theme.c
    src/tox.c
    src/tox_callbacks.c
    src/ui.c
    src/ui_i18n.c
    src/updater.c
    src/utox.c
    src/window.c

    ${WINDOWS_ICON}
    ${APPLE_ICON}

    )

target_link_libraries(utox
        utoxAV        utoxNATIVE      utoxUI
        ${TOX_LIBS}   ${LIBRARIES}    sodium
        vpx           pthread         m )

set_property(TARGET utox PROPERTY C_STANDARD 11)
if(APPLE)
    install(TARGETS utox
        RUNTIME DESTINATION "bin"
        BUNDLE  DESTINATION "resources"
        )
elseif(UNIX)
    install(TARGETS utox
        RUNTIME DESTINATION "bin"
        )
endif()
