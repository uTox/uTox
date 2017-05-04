project(utoxLAYOUT LANGUAGES C)

add_custom_command(OUTPUT icon.o
    COMMAND cd ${uTox_SOURCE_DIR}/ && ld -r -b binary -o ${utoxLAYOUT_BINARY_DIR}/icon.o icons/utox-128x128.png
    DEPENDS ../../icons/utox-128x128.png )
set_source_files_properties( icon.o PROPERTIES EXTERNAL_OBJECT true GENERATED true )
add_library(icon STATIC icon.o)
set_target_properties( icon PROPERTIES LINKER_LANGUAGE C )

add_library(utoxLAYOUT STATIC
    background.c
    friend.c
    group.c
    notify.c
    settings.c
    sidebar.c
    userbadge.c
    xlib_tray.c
    )

target_link_libraries(utoxLAYOUT
    icon
    )
