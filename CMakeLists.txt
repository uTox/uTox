cmake_minimum_required(VERSION 3.2)
project(uTox LANGUAGES C)

# This version is for the entire project. All libraries (core, av, ...) move in
# versions in a synchronised way.
set(PROJECT_VERSION_MAJOR "0")
set(PROJECT_VERSION_MINOR "10")
set(PROJECT_VERSION_PATCH "9")
set(PROJECT_VERSION
  "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fno-strict-aliasing")


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
    include_directories( libs/toktok/include libs/windows-x64/include/ )
    link_directories( libs/toktok/lib /usr/x86_64-w64-mingw32/lib/ libs/windows-x64/lib )
    #########################################
    ## Native Icon data
    #########################################
    enable_language(RC)

    #########################################
    ## Native Interface
    #########################################
    add_library(utoxNATIVE STATIC
        src/windows/audio.c
        src/windows/dnd.c
        src/windows/drawing.c
        src/windows/example.reg
        src/windows/filesys.c
        src/windows/main.7.c
        src/windows/main.c
        src/windows/main.h
        src/windows/main.XP.c
        src/windows/os_video.c
        src/windows/video.c
        )
    target_link_libraries(utoxNATIVE
        iphlpapi    ws2_32      gdi32       msimg32
        dnsapi      comdlg32    winmm       ole32
        oleaut32    strmiids    shell32     opus )

    set(WINDOWS_ICON icons/icon.rc)
elseif(APPLE) # Mac has a lot of dependencies
	add_definitions("-x objective-c")

    add_library(utoxNATIVE STATIC
        src/cocoa/MainMenu.xib
        src/cocoa/cursor.h
        src/cocoa/drawing.m
        src/posix/filesys.c
        src/cocoa/grabdesktop.m
        src/cocoa/interaction.m
        src/cocoa/main.h
        src/cocoa/main.m
        src/cocoa/objc_main.h
        src/cocoa/video.m
        )

    target_link_libraries(utoxNATIVE
        "-framework AppKit"
        "-framework ApplicationServices"
        "-framework AVFoundation"
        "-framework Cocoa"
        "-framework CoreData"
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework CoreMedia"
        "-framework CoreText"
        "-framework CoreVideo"
        "-framework Foundation"
        "-framework OpenAL"
        "-framework OpenGL"
        "-framework QuartzCore"
        -lresolv
        )

    set(APPLE_ICON utox.icns)
	add_custom_command(OUTPUT utox.icns
      COMMAND iconutil --convert icns src/cocoa/utox.iconset -o utox.icns
      DEPENDS src/cocoa/utox.iconset
	  )
elseif(UNIX) # xlib by default
    include_directories(/usr/include/freetype2)
    #########################################
    ## Native Icon data
    #########################################
    add_custom_command(OUTPUT icon.o
        COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/ && ld -r -b binary -o ${CMAKE_CURRENT_BINARY_DIR}/icon.o icons/utox-128x128.png
        DEPENDS icons/utox-128x128.png )
    set_source_files_properties( icon.o PROPERTIES EXTERNAL_OBJECT true GENERATED true )
    add_library(icon STATIC icon.o)
    set_target_properties( icon PROPERTIES LINKER_LANGUAGE C )
    #########################################
    ## Native Interface
    #########################################
    add_library(utoxNATIVE STATIC
        src/xlib/audio.c
        src/xlib/dbus.c
        src/xlib/dbus.h
        src/xlib/drawing.c
        src/xlib/event.c
        src/posix/filesys.c
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
        src/xlib/filesys.c
        )

    target_link_libraries(utoxNATIVE icon v4lconvert X11 Xext Xrender fontconfig freetype resolv dl )

	install(FILES
	  src/utox.desktop
	  DESTINATION "share/applications")
	install(FILES
	  src/utox.1
	  DESTINATION "man/man1")

endif()

#######################################################################
#
# :: Dependencies and configuration
#
#######################################################################

# TODO move this to the a new CMakeLists.txt in the ui subdir
add_library(utoxUI STATIC
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
    src/ui/tooltip.h )

#######################################################################
#
# :: uTox AV system (TODO move to subdir)
#
#######################################################################
add_library(utoxAV STATIC
    src/av
    src/av/utox_av.h
    src/av/utox_av.c
    src/av/audio.c
    src/av/audio.h
    src/av/video.h
    src/av/video.c
    )

if(NOT APPLE)
  target_link_libraries(utoxAV openal)
endif()


#######################################################################
#
# :: uTox main
#
#######################################################################
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
    src/stb_image_write.h
    src/theme.c
    src/theme.h
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

    ${WINDOWS_ICON}
    ${APPLE_ICON}

    )

target_link_libraries(utox
                      utoxAV        utoxNATIVE      utoxUI
                      ${TOX_LIBS}   ${LIBRARIES}
                      sodium        pthread     m   vpx )

set_property(TARGET utox PROPERTY C_STANDARD 99)
if(UNIX)
	install(TARGETS utox RUNTIME DESTINATION "bin")
endif()
