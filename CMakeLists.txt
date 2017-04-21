cmake_minimum_required(VERSION 3.0)
project(uTox LANGUAGES C)

#######################################################################
## Project Version
#######################################################################
set(PROJECT_VERSION_MAJOR "0")
set(PROJECT_VERSION_MINOR "14")
set(PROJECT_VERSION_PATCH "0")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")


###########
# Options #
###########

option(ENABLE_ASAN          "Enable Address Sanitizer on debug builds"                      ON )
option(ENABLE_TESTS         "Whether to build test binaries (currently linux only)"         ON )
option(ENABLE_WERROR        "Error on Warning, whether to put -Werror flag to the compiler" OFF)
option(ENABLE_FILTERAUDIO   "Enable Filter Audio"                                           ON )
option(ENABLE_AUTOUPDATE    "Enable Auto-updater"                                           OFF)

option(UTOX_STATIC          "Link uTox statically"                                          OFF)
option(TOXCORE_STATIC       "Build uTox with the static version of Toxcore"                 OFF)

option(ENABLE_LTO           "Enable link time optimizations"                                ON )


########################
# Set helper-variables #
########################

# OS
if(CYGWIN)
    # Cygwin thinks it's actually Linux.
    set(WIN32 TRUE)
    set(UNIX FALSE)
endif()

if(WIN32)
    set(WINDOWS TRUE)
elseif(APPLE)
    if(CMAKE_SYSTEM_NAME MATCHES ".*MacOS.*")
        set(MACOSX TRUE)
    endif()
elseif(UNIX)
    if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
        set(LINUX TRUE)
    elseif(CMAKE_SYSTEM_NAME MATCHES "kOpenBSD.*|OpenBSD.*")
        set(OPENBSD TRUE)
    elseif(CMAKE_SYSTEM_NAME MATCHES "kNetBSD.*|NetBSD.*")
        set(NETBSD TRUE)
    elseif(CMAKE_SYSTEM_NAME MATCHES "kFreeBSD.*|FreeBSD")
        set(FREEBSD TRUE)
    endif()
endif()

# 32 or 64 bit
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(ARCH_64 TRUE)
else()
  set(ARCH_64 FALSE)
endif()


#################################
# Include toolchain if required #
#################################

if(WINDOWS AND NOT CROSS_COMPILING)
    include(cmake/win.cmake)

    if(ARCH_64)
        # Helping Cygwin out again..
        set(WIN64 TRUE)
    endif()
elseif(FREEBSD)
    include(cmake/toolchain-freebsd.cmake)
elseif(OPENBSD OR NETBSD)
    include(cmake/toolchain-openbsd.cmake)
endif()


#######################################################################
# CMake Settings
#######################################################################
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

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
# C compiler flags
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
## Add all warning flags we can.
add_cflag("-Wall")
add_cflag("-Wextra")
add_cflag("-Wpointer-arith")

## Warnings we don't tolerate
add_cflag("-Werror=implicit-function-declaration")

## Warnings we don't care about
add_cflag("-Wformat=0")
add_cflag("-Wno-misleading-indentation")

## Everything Else
add_cflag("-fno-strict-aliasing")
add_cflag("-fPIC")

## Optimization Flags

# Fix GNU stack
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    # Have ld strip the symbols from Release and MinSizeRel build types.
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os")
    set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Os")

    if(LINUX)
        # enable Link Time Optimization on gcc (linux only currently)
        # TODO someone should figure out how this works on windows, or we need to have gitlab-ci for GCC
        if(ENABLE_LTO)
            add_cflag("-flto")
        endif()

        set(CMAKE_AR "gcc-ar")
        set(CMAKE_RANLIB "gcc-ranlib")

        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,noexecstack")
    endif()
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    # enable Link Time Optimization on clang
    if(ENABLE_LTO)
        add_cflag("-flto")
    endif()

    # Have ld strip the symbols from Release and MinSizeRel build types. (-Oz is clang specific)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os")
    set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Oz")

    if(NOT ENABLE_ASAN)
        if(NOT APPLE)
            add_cflag("-fsanitize=safe-stack")
        endif()
    endif()
else()
    message(SEND_ERROR "Unknown compiler name")
endif()

# set define for GIT_VERSION
if(GIT_VERSION)
    add_cflag("-DGIT_VERSION='\"${GIT_VERSION}\"'")
endif()

#######################################################################
# Build options
#######################################################################
# sanitize memory issues in Debug builds
# https://gcc.gnu.org/gcc-4.8/changes.html
# https://github.com/google/sanitizers/wiki/AddressSanitizer
if (ENABLE_ASAN)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")
endif()

if(ENABLE_WERROR)
    add_cflag("-Werror")
endif()

if(ENABLE_FILTERAUDIO)
    add_cflag("-DAUDIO_FILTERING")
endif()

if(UTOX_STATIC)
    if(UNIX)
        add_cflag("-Wl -Bstatic -lopus -lopenal -lfilteraudio -Bdynamic -lrt -lm -lpthread -ldl")
    else()
        add_cflag("-static")
    endif()
endif()

if(ENABLE_AUTOUPDATE)
    add_cflag("-DENABLE_AUTOUPDATE=1")
endif()

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

# include utoxLAYOUT
add_subdirectory(src/layout)
# include utoxUI
add_subdirectory(src/ui)
# include utoxAV
add_subdirectory(src/av)

#######################################################################
# :: uTox main
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

#######################################################################
#
# Testing
#
#######################################################################
if(ENABLE_TESTS)
  if(UNIX)
    # tests currently only run on linux
    enable_testing()
    add_subdirectory(tests)
  endif()
endif()

# print a summary of build options
message("Build options:")
message("- CMake Module Path:        ${CMAKE_MODULE_PATH}")
message("----------------------------------")
message("- Add ASAN:                ${ENABLE_ASAN}")
message("- Error on Warning:        ${ENABLE_WERROR}")
message("- Filter Audio:            ${ENABLE_FILTERAUDIO}")
message("- Auto Updater:            ${ENABLE_AUTOUPDATE}")
message("- uTox Static:             ${UTOX_STATIC}")
message("- Toxcore Static:          ${TOXCORE_STATIC}")
message("-- Platform Options --------------")
message("- Windows Legacy:          ${WIN_XP_LEGACY}")
message("- Enable DBus:             ${ENABLE_DBUS}")
message("- Enable Tests             ${ENABLE_TESTS}")

message("* CMAKE SYSTEM is '${CMAKE_SYSTEM_NAME}'")
message("* C Compiler is '${CMAKE_C_COMPILER}' with the following flags:")
message("* C flags for Debug:       ${CMAKE_C_FLAGS_DEBUG}")
message("* C flags for Release:     ${CMAKE_C_FLAGS_RELEASE}")
message("* C flags for all types:   ${CMAKE_C_FLAGS}")

if (ENABLE_ASAN)
    message("")
    message("  TIP: You may want to set the env variable  ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer  for ASAN to show nicer stack traces.")
    message("       See <http://clang.llvm.org/docs/AddressSanitizer.html#symbolizing-the-reports> for more details.")
    message("")
endif()
