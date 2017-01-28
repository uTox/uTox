project(utoxNATIVE LANGUAGES C)

option(ENABLE_DBUS "Compile with dbus notification support" ON)

if(ENABLE_DBUS)
    include_directories(${DBUS_INCLUDE_DIR} "/usr/lib/dbus-1.0/include")
    add_cflag("-DHAVE_DBUS=1")
else()
    add_cflag("-DNO_DBUS=1")
endif()

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

find_package(X11 REQUIRED)
message("X include:         ${X11_INCLUDE_DIR}")
message("X library:         ${X11_LIBRARIES}")
message("Xrender include:   ${X11_Xrender_INCLUDE_PATH}")
message("Xrender library:   ${X11_Xrender_LIB}")

message("DBus include:      ${DBUS_INCLUDE_DIR}")
message("DBus library:      ${DBUS_LIBRARIES}")

target_link_libraries(utoxNATIVE icon v4lconvert ${X11_LIBRARIES} ${X11_Xrender_LIB} fontconfig ${FREETYPE_LIBRARIES} resolv dl ${DBUS_LIBRARIES})

install(FILES
    ../utox.desktop
    DESTINATION "share/applications"
    )

install(FILES
    ../utox.1
    DESTINATION "share/man/man1"
    )
