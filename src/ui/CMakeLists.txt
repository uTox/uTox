project(utoxUI LANGUAGES C)

add_library(utoxUI STATIC
    button.c
    contextmenu.c
    draw.c
    dropdown.c
    edit.c
    scrollable.c
    svg.c
    switch.c
    text.c
    tooltip.c
    )

target_link_libraries(utoxUI utoxLAYOUT)
