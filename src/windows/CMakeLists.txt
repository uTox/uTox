cmake_minimum_required(VERSION 3.2)
project(utoxNATIVE LANGUAGES C)

include_directories( libs/toktok/include libs/windows-x64/include/ )
link_directories( libs/toktok/lib /usr/x86_64-w64-mingw32/lib/ libs/windows-x64/lib )

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
    example.reg
    filesys.c
    main.7.c
    main.c
    main.h
    main.XP.c
    os_video.c
    video.c
    )

target_link_libraries(utoxNATIVE
    iphlpapi    ws2_32      gdi32       msimg32
    dnsapi      comdlg32    winmm       ole32
    oleaut32    strmiids    shell32     opus )

set(WINDOWS_ICON icons/icon.rc)
