cmake_minimum_required(VERSION 3.2)
project(uTox LANGUAGES C)

if(WIN32)
    add_subdirectory(windows)
elseif(APPLE)
    add_subdirectory(cocoa)
elseif(UNIX)
    add_subdirectory(xlib)
endif()

add_subdirectory(ui)
add_subdirectory(av)

#######################################################################
#
# :: uTox main
#
#######################################################################
add_executable(utox
    avatar.c
    avatar.h
    chatlog.c
    chatlog.h
    command_funcs.c
    command_funcs.h
    commands.c
    commands.h
    devices.c
    devices.h
    dns.c
    dns.h
    file_transfers.c
    file_transfers.h
    flist.c
    flist.h
    friend.c
    friend.h
    groups.c
    groups.h
    inline_video.c
    inline_video.h
    main.c
    main.h
    main_native.h
    messages.c
    messages.h
    sized_string.h
    stb_image.h
    stb_image_write.h
    theme.c
    theme.h
    theme_tables.h
    tox.c
    tox.h
    utox.c
    utox.h
    tox_callbacks.c
    tox_callbacks.h
    ui.c
    ui.h
    ui_i18n.c
    ui_i18n.h
    util.c
    util.h

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
