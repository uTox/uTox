project(utoxUI LANGUAGES C)

add_library(utoxUI STATIC
    ../layout/layout_all.h
    ../layout/layout_notify.h
    ../layout/layout_root.h

    ../layout/tree/notify.c

    button.c
    button.h
    buttons.c
    buttons.h
    contextmenu.c
    contextmenu.h
    draw.h
    draw_helpers.c
    draw_helpers.h
    dropdown.c
    dropdown.h
    dropdowns.c
    dropdowns.h
    edit.c
    edit.h
    edits.c
    edits.h
    scrollable.c
    scrollable.h
    svg.c
    svg.h
    switch.c
    switch.h
    switches.c
    switches.h
    text.c
    text.h
    tooltip.c
    tooltip.h
    )
