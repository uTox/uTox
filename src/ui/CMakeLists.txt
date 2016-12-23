cmake_minimum_required(VERSION 3.2)
project(utoxUI LANGUAGES C)

if(UNIX)
    include_directories(/usr/include/freetype2)
endif()

add_library(utoxUI STATIC
    button.c
    button.h
    buttons.c
    buttons.h
    contextmenu.c
    contextmenu.h
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
    layout_tree.h
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
