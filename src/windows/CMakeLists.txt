project(utoxNATIVE LANGUAGES C)

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
    utf8.c
    video.c
    window.c

    main.7.c
    )

target_link_libraries(utoxNATIVE
    iphlpapi    ws2_32      gdi32       msimg32
    dnsapi      comdlg32    winmm       ole32
    oleaut32    strmiids    shell32     opus
    )

set(WINDOWS_ICON utox.rc)
