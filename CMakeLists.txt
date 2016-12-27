cmake_minimum_required(VERSION 3.2)
project(uTox LANGUAGES C)

# This version is for the entire project. All libraries (core, av, ...) move in
# versions in a synchronised way.
set(PROJECT_VERSION_MAJOR "0")
set(PROJECT_VERSION_MINOR "11")
set(PROJECT_VERSION_PATCH "1")
set(PROJECT_VERSION
  "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
# make version available in C files
configure_file(${uTox_SOURCE_DIR}/src/branding.h.in
               ${uTox_BINARY_DIR}/src/branding.h)
include_directories("${uTox_BINARY_DIR}/src")



set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fno-strict-aliasing")

option(FILTER_AUDIO "Enable Filter Audio" ON)
if(FILTER_AUDIO)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DAUDIO_FILTERING")
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
message("- Filter Audio: ${FILTER_AUDIO}")
message("- Toxcore Static: ${TOXCORE_STATIC}")


# include utoxNATIVE dependend on the OS
if(WIN32)
    ############################################################
    ## This is a hack, that we can remove once we have better
    ## separation of the sections of uTox, until then, this
    ## makes everything work.
    ############################################################
    include_directories( libs/toktok/include libs/windows-x64/include/ )
    link_directories( libs/toktok/lib /usr/x86_64-w64-mingw32/lib/ libs/windows-x64/lib )

    ## Needed to build the widows icon
    enable_language(RC)
    set(WINDOWS_ICON icons/icon.rc)

    add_subdirectory(src/windows)
elseif(APPLE)
    add_definitions("-x objective-c")
    add_subdirectory(src/cocoa)
elseif(UNIX)
    find_package(Freetype REQUIRED)
    include_directories(${FREETYPE_INCLUDE_DIRS})
    message("Found Freetype version ${FREETYPE_VERSION_STRING}")
    message("Freetype include: ${FREETYPE_INCLUDE_DIRS}")
    message("Freetype library: ${FREETYPE_LIBRARIES}")

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
add_executable(utox
    src/avatar.c
    src/avatar.h
    src/chatlog.c
    src/chatlog.h
    src/command_funcs.c
    src/command_funcs.h
    src/commands.c
    src/commands.h
    src/devices.c
    src/devices.h
    src/dns.c
    src/dns.h
    src/file_transfers.c
    src/file_transfers.h
    src/flist.c
    src/flist.h
    src/friend.c
    src/friend.h
    src/groups.c
    src/groups.h
    src/inline_video.c
    src/inline_video.h
    src/main.c
    src/main.h
    src/main_native.h
    src/messages.c
    src/messages.h
    src/sized_string.h
    src/stb_image.h
    src/stb_image_write.h
    src/theme.c
    src/theme.h
    src/theme_tables.h
    src/tox.c
    src/tox.h
    src/utox.c
    src/utox.h
    src/tox_callbacks.c
    src/tox_callbacks.h
    src/ui.c
    src/ui.h
    src/ui_i18n.c
    src/ui_i18n.h
    src/util.c
    src/util.h

    ${WINDOWS_ICON}
    ${APPLE_ICON}

    )

target_link_libraries(utox
        utoxAV        utoxNATIVE      utoxUI
        ${TOX_LIBS}   ${LIBRARIES}    sodium
        vpx           pthread         m )

set_property(TARGET utox PROPERTY C_STANDARD 99)
if(UNIX)
    install(TARGETS utox RUNTIME DESTINATION "bin")
endif()
