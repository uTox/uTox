project(utoxUI LANGUAGES C)

add_library(utoxUI STATIC
    ../layout/friend.c
    ../layout/group.c
    ../layout/notify.c
    ../layout/settings.c
    ../layout/sidebar.c
    ../layout/userbadge.c
    ../layout/background.c

    ../layout/extra.c

    button.c
    contextmenu.c
    contextmenu.h
    draw.h
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
    text.c
    text.h
    tooltip.c
    tooltip.h
    )
