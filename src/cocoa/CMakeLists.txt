project(utoxNATIVE LANGUAGES C)

add_library(utoxNATIVE STATIC
    ../posix/filesys.c

    cursor.h
    drawing.m
    grabdesktop.m
    interaction.m
    main.h
    main.m
    MainMenu.xib
    objc_main.h
    video.m

    window.c
    )

target_link_libraries(utoxNATIVE
    "-framework AppKit"
    "-framework ApplicationServices"
    "-framework AVFoundation"
    "-framework Cocoa"
    "-framework CoreData"
    "-framework CoreFoundation"
    "-framework CoreGraphics"
    "-framework CoreMedia"
    "-framework CoreText"
    "-framework CoreVideo"
    "-framework Foundation"
    "-framework OpenAL"
    "-framework OpenGL"
    "-framework QuartzCore"
    -lresolv
    )
