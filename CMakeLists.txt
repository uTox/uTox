cmake_minimum_required(VERSION 3.2)
project(uTox LANGUAGES C)

# This version is for the entire project. All libraries (core, av, ...) move in
# versions in a synchronised way.
set(PROJECT_VERSION_MAJOR "0")
set(PROJECT_VERSION_MINOR "10")
set(PROJECT_VERSION_PATCH "9")
set(PROJECT_VERSION
  "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

set(LIBRARIES toxcore toxav toxencryptsave toxdns openal m pthread vpx )

# Protip, you must use a different directory for each build target...
# -DMAKE_TOOLCHAIN_FILE has no effect unless the target directory is empty
# 1.5 hours to learn this...
if(WIN32)
    set(U_LIBRARIES utoxAV win_Native uUI )
    list(APPEND LIBRARIES)

    add_library(win_Native STATIC
        src/windows/audio.c
        src/windows/dnd.c
        src/windows/drawing.c
        src/windows/example.reg
        src/windows/main.7.c
        src/windows/main.c
        src/windows/main.h
        src/windows/main.XP.c
        src/windows/os_video.c
        src/windows/video.c
        )
    target_link_libraries(win_Native icon)
elseif(APPLE)
    set(U_LIBRARIES utoxAV cocoa_Native uUI )
    list(APPEND LIBRARIES)
elseif(UNIX) # xlib by default
    set(U_LIBRARIES utoxAV xlib_Native uUI )
    list(APPEND LIBRARIES v4lconvert X11 Xext Xrender fontconfig freetype resolv dl )

    add_library(xlib_Native STATIC
        src/xlib/audio.c
        src/xlib/dbus.c
        src/xlib/dbus.h
        src/xlib/drawing.c
        src/xlib/event.c
        src/xlib/freetype.c
        src/xlib/freetype.h
        src/xlib/gtk.c
        src/xlib/keysym2ucs.h
        src/xlib/main.c
        src/xlib/main.h
        src/xlib/mmenu.c
        src/xlib/mmenu.h
        src/xlib/v4l.c
        src/xlib/video.c
        )
    target_link_libraries(xlib_Native icon)

endif()

# if(NOT CMAKE_CROSSCOMPILING)
#     set(CMAKE_C_COMPILER clang)
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -fno-strict-aliasing")
# endif()
################################################################################
#
# :: Dependencies and configuration
#
################################################################################
include_directories(/usr/include/freetype2)

# TODO move this to the a new CMakeLists.txt in the ui subdir
add_library(uUI STATIC
    src/ui/button.c
    src/ui/button.h
    src/ui/buttons.c
    src/ui/buttons.h
    src/ui/contextmenu.c
    src/ui/contextmenu.h
    src/ui/draw_helpers.c
    src/ui/draw_helpers.h
    src/ui/dropdown.c
    src/ui/dropdown.h
    src/ui/dropdowns.c
    src/ui/dropdowns.h
    src/ui/edit.c
    src/ui/edit.h
    src/ui/edits.c
    src/ui/edits.h
    src/ui/layout_tree.h
    src/ui/scrollable.c
    src/ui/scrollable.h
    src/ui/svg.c
    src/ui/svg.h
    src/ui/switch.c
    src/ui/switch.h
    src/ui/switches.c
    src/ui/switches.h
    src/ui/text.c
    src/ui/text.h
    src/ui/tooltip.c
    src/ui/tooltip.h
    )


add_library(utoxAV STATIC
    src/av
    src/av/utox_av.h
    src/av/utox_av.c
    src/av/audio.c
    src/av/audio.h
    src/av/video.h
    src/av/video.c
    )

add_executable(utox
    src/avatar.c
    src/avatar.h
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
    src/stb_image.h
    src/stb_image_write.h
    src/stb_image_write.h
    src/theme.c
    src/theme.h
    src/theme_tables.h
    src/theme_tables.h
    src/tox.c
    src/tox.h
    src/tox_callbacks.c
    src/tox_callbacks.h
    src/ui.c
    src/ui.h
    src/ui_i18n.c
    src/ui_i18n.h
    src/util.c
    src/util.h
    )

add_custom_command(OUTPUT icon.o
    COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/ && ld -r -b binary -o ${CMAKE_CURRENT_BINARY_DIR}/icon.o icons/utox-128x128.png
    COMMAND objcopy --rename-section .data=.rodata,alloc,load,readonly,data,contents ${CMAKE_CURRENT_BINARY_DIR}/icon.o ${CMAKE_CURRENT_BINARY_DIR}/icon.o
    DEPENDS icons/icon.rc icons/utox-128x128.png
    )

SET_SOURCE_FILES_PROPERTIES( icon.o PROPERTIES EXTERNAL_OBJECT true GENERATED true )
add_library(icon STATIC icon.o)
SET_TARGET_PROPERTIES( icon PROPERTIES LINKER_LANGUAGE C )


target_link_libraries(utox
    ${LIBRARIES}
    ${U_LIBRARIES}
)
