project(utoxUI LANGUAGES C)

add_library(utoxUI STATIC
    button.c
    contextmenu.c
    contextmenu.h
    draw.h
    dropdown.c
    dropdown.h
    edit.c
    edit.h
    scrollable.c
    scrollable.h
    svg.c
    svg.h
    switch.c
    switch.h
    text.c
    text.h
    tooltip.c
    tooltip.h
    )

target_link_libraries(utoxUI utoxLAYOUT)
