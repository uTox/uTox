project(utoxNATIVE LANGUAGES C)

add_library(utoxNATIVE STATIC
    ../posix/filesys.c

    drawing.m
    grabdesktop.m
    interaction.m
    main.m
    MainMenu.xib
    video.m

    window.c
    )

target_link_libraries(utoxNATIVE
    PUBLIC
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
    PRIVATE
        stb
)
