project(utoxNATIVE LANGUAGES C)

option(WIN_XP_LEGACY "Compile Legacy code for Windows XP" OFF)
if(WIN_XP_LEGACY)
    add_cflag("-D__WIN_LEGACY=1")
endif()

#########################################
## Native Icon data
#########################################
enable_language(RC)

#########################################
## Native Interface
#########################################
add_library(utoxNATIVE STATIC
    audio.c
    dnd.c
    drawing.c
    events.c
    filesys.c
    main.c
    main.h
    notify.c
    os_video.c
    screen_grab.c
    video.c
    window.c

    ## The following two have ifdef's for __WIN_LEGACY
    #  We include them both here even though only one will compile with content
    main.XP.c
    main.7.c
    )

target_link_libraries(utoxNATIVE
    iphlpapi    ws2_32      gdi32       msimg32
    dnsapi      comdlg32    winmm       ole32
    oleaut32    strmiids    shell32     opus
    )

set(WINDOWS_ICON icons/icon.rc)
