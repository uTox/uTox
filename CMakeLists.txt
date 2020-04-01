cmake_minimum_required(VERSION 3.2)
project(uTox LANGUAGES C)

#####################
## Project Metadata #
#####################

set(PROJECT_VERSION_MAJOR "0")
set(PROJECT_VERSION_MINOR "17")
set(PROJECT_VERSION_PATCH "1")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

set(PROJECT_COPYRIGHT "Copyleft 2019 uTox contributors. Some rights reserved.")

string(TIMESTAMP BUILD_YEAR "%Y" UTC)
string(TIMESTAMP BUILD_MONTH "%B" UTC)

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
        set(CMAKE_OSX_DEPLOYMENT_TARGET 10.6 CACHE STRING "Minimum OS X deployment version")
        set(MACOSX TRUE)
    endif()
elseif(UNIX)
    if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
        set(LINUX TRUE)
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



#############################
# System-dependent defaults #
#############################

# Windows only compiles statically.
if (WINDOWS)
    set(STATIC_DEFAULT ON)
else()
    set(STATIC_DEFAULT OFF)
endif()

# Disable ASAN for macOS due to "AddressSanitizer: odr-violation: global". see #957
# Windows doesn't compile with asan enabled.
if (APPLE OR WINDOWS)
    set(ASAN_DEFAULT OFF)
else()
    set(ASAN_DEFAULT ON)
endif()

# ASAN causes uTox to be slower so do no use it for release builds
if (CMAKE_BUILD_TYPE MATCHES "Release")
   set(ASAN_DEFAULT OFF)
endif()

###########
# Options #
###########

option(STATIC_ALL         "Enable all STATIC_* options"                          ${STATIC_DEFAULT} )
option(STATIC_TOXCORE     "Link toxcore statically"                              ${STATIC_DEFAULT} )
option(STATIC_DBUS        "Link dbus statically"                                 ${STATIC_DEFAULT} )
option(STATIC_FILTERAUDIO "Link filteraudio statically"                          ${STATIC_DEFAULT} )
option(STATIC_OPUS        "Link opus statically"                                 ${STATIC_DEFAULT} )
option(STATIC_SODIUM      "Link sodium statically"                               ${STATIC_DEFAULT} )
option(STATIC_VPX         "Link vpx statically"                                  ${STATIC_DEFAULT} )
option(ENABLE_ASAN        "Enable Address Sanitizer on debug builds"               ${ASAN_DEFAULT} )
option(ENABLE_TESTS       "Whether to build test binaries (currently Unix-like OSes only)"      ON )
option(ENABLE_WERROR      "Error on Warning, whether to put -Werror flag to the compiler"       OFF)
option(ENABLE_FILTERAUDIO "Enable Filter Audio"                                                 ON )
option(ENABLE_AUTOUPDATE  "Enable Auto-updater"                                                 OFF)
option(ENABLE_LTO         "Enable link time optimizations"                                      ON )


#################################
# Include toolchain if required #
#################################

if(WINDOWS AND NOT CROSS_COMPILING)
    include(cmake/win.cmake)

    if(ARCH_64)
        # Helping Cygwin out again..
        set(WIN64 TRUE)
    endif()
elseif(APPLE)
    include(cmake/macOS.cmake)
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
configure_file(${uTox_SOURCE_DIR}/src/utox.1.in
               ${uTox_SOURCE_DIR}/src/utox.1)

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
    else()
        message(STATUS "Warning: Can't find flag ${flag}")
    endif()
endfunction()

# Warn on non-ISO C.
#add_cflag("-pedantic") # throws a lot of warning, quite sure we do not want this at current state.
## Add all warning flags we can.
add_cflag("-Wall")
add_cflag("-Wextra")
add_cflag("-Wpointer-arith")
add_cflag("-Wimplicit-fallthrough=5")

## Warnings we don't tolerate
add_cflag("-Werror=implicit-function-declaration")

## Warnings we don't care about
add_cflag("-Wformat=0")
add_cflag("-Wno-misleading-indentation")

## Everything Else
add_cflag("-fno-strict-aliasing")
add_cflag("-fPIC")
if(NOT WINDOWS) # TODO fix windows
    add_cflag("-fno-common")
endif()

## Optimization Flags

# Fix GNU stack
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    # Have ld strip the symbols from Release and MinSizeRel build types.
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os")
    set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Os -s")

    if(LINUX)
        # enable Link Time Optimization on gcc (linux only currently)
        # TODO someone should figure out how this works on windows, or we need to have gitlab-ci for GCC
        if(ENABLE_LTO)
            ## -flto is a flag that needs to exist at link time as well,
            #  so we use some hacky save/restore to help cmake.
            set(SAVE_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
            set(CMAKE_REQUIRED_LIBRARIES "-flto")
            add_cflag("-flto")
            set(CMAKE_REQUIRED_LIBRARIES "${SAVE_CMAKE_REQUIRED_LIBRARIES}")
        endif()

        set(CMAKE_AR "gcc-ar")
        set(CMAKE_RANLIB "gcc-ranlib")

        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,noexecstack")
    endif()
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    # Disable warning for `= { 0 }` in Clang.
    # Remove once they've resolved https://bugs.llvm.org//show_bug.cgi?id=21689
    add_cflag("-Wno-missing-field-initializers")

    # Enable Link Time Optimization on Clang
    if(ENABLE_LTO)
        ## -flto is a flag that needs to exist at link time as well,
        #  so we use some hacky save/restore to help cmake.
        set(SAVE_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
        set(CMAKE_REQUIRED_LIBRARIES "-flto")
        add_cflag("-flto")
        set(CMAKE_REQUIRED_LIBRARIES "${SAVE_CMAKE_REQUIRED_LIBRARIES}")
    endif()

    # Have ld strip the symbols from Release and MinSizeRel build types. (-Oz is clang specific)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os")
    set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Oz -s")

    if(FREEBSD)
        set(LIBRARIES ${LIBRARIES} "cxxrt")
    endif()

    if(NOT ENABLE_ASAN)
        if(NOT APPLE)
            add_cflag("-fsanitize=safe-stack")
        endif()
    endif()
else()
    message(STATUS "Unknown compiler name: You may have to set up your compiler flags yourself.")
endif()

# set define for GIT_VERSION
if(GIT_VERSION)
    add_cflag("-DGIT_VERSION='\"${GIT_VERSION}\"'")
endif()


#######################################################################
# Build options
#######################################################################

# https://github.com/google/sanitizers/wiki/AddressSanitizer
if (ENABLE_ASAN)
    set(SAVE_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
    set(CMAKE_REQUIRED_LIBRARIES "-fsanitize=address")
    add_cflag("-fsanitize=address")
    add_cflag("-fno-omit-frame-pointer")
    set(CMAKE_REQUIRED_LIBRARIES "${SAVE_CMAKE_REQUIRED_LIBRARIES}")
endif()

if(ENABLE_WERROR)
    add_cflag("-Werror")
endif()

if(STATIC_ALL)
    if(UNIX)
        add_cflag("-Wl -Bstatic -lopus -lopenal -lfilteraudio -Bdynamic -lrt -lm -lpthread -ldl")
    else()
        add_cflag("-static")
    endif()
endif()

if(ENABLE_AUTOUPDATE)
    add_cflag("-DENABLE_AUTOUPDATE=1")
endif()

if(ENABLE_FILTERAUDIO)
    add_cflag("-DAUDIO_FILTERING=1")
endif()

find_package(libtox REQUIRED)
include_directories(${LIBTOX_INCLUDE_DIRS})
set(LIBRARIES ${LIBRARIES} ${LIBTOX_LIBRARIES})

find_package(libsodium REQUIRED)
include_directories(${LIBSODIUM_INCLUDE_DIRS})
set(LIBRARIES ${LIBRARIES} ${LIBSODIUM_LIBRARIES})

find_package(libvpx REQUIRED)
include_directories(${LIBVPX_INCLUDE_DIRS})
set(LIBRARIES ${LIBRARIES} ${LIBVPX_LIBRARIES})

include_directories(SYSTEM third-party/stb)
include_directories(SYSTEM third-party/minini/dev)
include_directories(SYSTEM third-party/qrcodegen/c)

# Protip, you must use a different directory for each build target...
# -DMAKE_TOOLCHAIN_FILE has no effect unless the target directory is empty
# 1.5 hours to learn this...
if(WIN32)
    if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        set(GUI_TYPE WIN32)
    endif()

    link_directories(${CMAKE_PREFIX_PATH}/lib)

    # Windows icons
    enable_language(RC)
    set(WINDOWS_ICON src/windows/utox.rc)

    add_subdirectory(src/windows)
elseif(APPLE)
    set(GUI_TYPE MACOSX_BUNDLE)

    add_definitions("-x objective-c")
    add_subdirectory(src/cocoa)
elseif(UNIX)
    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})

    add_subdirectory(src/xlib)

    if(LINUX)
        # Required for v4l (at least in the Linux static builds on Jenkins)
        set(LIBRARIES ${LIBRARIES} "rt")
    endif()
endif()

# Acquire 3rd party headers
find_path(STBI_INCLUDE_DIR NAMES stb_image.h stb_image_write.h PATHS third-party/stb)
if((NOT STBI_INCLUDE_DIR) OR (NOT EXISTS ${STBI_INCLUDE_DIR}))
    message("STBI not found")
    execute_process(COMMAND git submodule update --init -- third-party/stb WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    # set FOO_INCLUDE_DIR properly
    set(STBI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/stb/ CACHE PATH "stbi include directory")
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
    src/chrono.c
    src/command_funcs.c
    src/commands.c
    src/devices.c
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
    src/qr.c
    src/screen_grab.c
    src/self.c
    src/settings.c
    src/sized_string.h
    src/stb.c
    src/text.c
    src/theme.c
    src/theme_tables.c
    src/tox.c
    src/tox_callbacks.c
    src/ui.c
    src/ui_i18n.c
    src/updater.c
    src/utox.c
    src/window.c
    third-party/minini/dev/minIni.c
    third-party/qrcodegen/c/qrcodegen.c

    ${WINDOWS_ICON}
    ${APPLE_FILES}
)

target_link_libraries(utox
    utoxAV
    utoxNATIVE
    utoxUI
    ${LIBRARIES}
    pthread
    m
)

set_property(TARGET utox PROPERTY C_STANDARD 11)

if(APPLE)
    set_target_properties(utox PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${uTox_SOURCE_DIR}/src/cocoa/Info.plist")
    # check that the app is valid and when the app is static
    # also check that it does not depend on external libs (system libs excepted)
    install(CODE "include(BundleUtilities)
        verify_app(${CMAKE_BINARY_DIR}/utox.app)
        if(STATIC_ALL OR STATIC_TOXCORE)
            verify_bundle_prerequisites(${CMAKE_BINARY_DIR}/utox.app RETURN_VAR INFO_VAR)
        endif()"
    )
    # needed for packaging, so that CPack can find the app
    install(TARGETS utox DESTINATION .)
    # install App
    install(TARGETS utox DESTINATION /Applications)
elseif(UNIX)
    install(TARGETS utox
        RUNTIME DESTINATION "bin"
        )
endif()

# packaging
include(CPack)

###########
# Testing #
###########

if(ENABLE_TESTS)
  if(UNIX)
    # Tests currently only run on Unix-like OSes.
    enable_testing()
    add_subdirectory(tests)
  endif()
endif()


#############
# Info dump #
#############

message("Build options:")
message("- CMake Module Path:        ${CMAKE_MODULE_PATH}")
message("----------------------------------")
message("- Add ASAN:                ${ENABLE_ASAN}")
message("- Link-Time Optimization:  ${ENABLE_LTO}")
message("- Error on Warning:        ${ENABLE_WERROR}")
message("- Filter Audio:            ${ENABLE_FILTERAUDIO}")
message("- Auto Updater:            ${ENABLE_AUTOUPDATE}")
message("- Static uTox:             ${STATIC_ALL}")
message("- Static Toxcore:          ${STATIC_TOXCORE}")
message("- Static DBus:             ${STATIC_DBUS}")
message("- Static Filteraudio:      ${STATIC_FILTERAUDIO}")
message("- Static Opus:             ${STATIC_OPUS}")
message("- Static Sodium:           ${STATIC_SODIUM}")
message("- Static VPX:              ${STATIC_VPX}")
message("-- Platform Options --------------")
message("- Enable DBus:             ${ENABLE_DBUS}")
message("- Enable Tests             ${ENABLE_TESTS}")

message("* CMake system is '${CMAKE_SYSTEM_NAME}'")
message("* CMake build type is '${CMAKE_BUILD_TYPE}'")
message("* C Compiler is '${CMAKE_C_COMPILER}' with the following flags:")
message("* C flags for Debug:       ${CMAKE_C_FLAGS_DEBUG}")
message("* C flags for Release:     ${CMAKE_C_FLAGS_RELEASE}")
message("* C flags for all types:   ${CMAKE_C_FLAGS}")

if(ENABLE_ASAN)
    message("")
    message("  TIP: You may want to set the env variable  ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer  for ASAN to show nicer stack traces.")
    message("       See <http://clang.llvm.org/docs/AddressSanitizer.html#symbolizing-the-reports> for more details.")
    message("")
endif()
