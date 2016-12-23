cmake_minimum_required(VERSION 3.2)
project(utoxNATIVE LANGUAGES C)

include_directories(/usr/include/freetype2)

#########################################
## Native Icon data
#########################################
add_custom_command(OUTPUT icon.o
    COMMAND cd ${uTox_SOURCE_DIR}/ && ld -r -b binary -o ${utoxNATIVE_BINARY_DIR}/icon.o icons/utox-128x128.png
    DEPENDS ../../icons/utox-128x128.png )
set_source_files_properties( icon.o PROPERTIES EXTERNAL_OBJECT true GENERATED true )
add_library(icon STATIC icon.o)
set_target_properties( icon PROPERTIES LINKER_LANGUAGE C )

#########################################
## Native Interface
#########################################
add_library(utoxNATIVE STATIC
    audio.c
    dbus.c
    dbus.h
    drawing.c
    event.c
    ../posix/filesys.c
    freetype.c
    freetype.h
    gtk.c
    keysym2ucs.h
    main.c
    main.h
    mmenu.c
    mmenu.h
    v4l.c
    video.c
    filesys.c
    )

target_link_libraries(utoxNATIVE icon v4lconvert X11 Xext Xrender fontconfig freetype resolv dl )

install(FILES
	../utox.desktop
	DESTINATION "share/applications"
    )

install(FILES
	../utox.1
	DESTINATION "man/man1"
    )
