cmake_minimum_required(VERSION 3.2)
project(uTox LANGUAGES C)

add_definitions("-x objective-c")

add_library(utoxNATIVE STATIC
    MainMenu.xib
    cursor.h
    drawing.m
    ../posix/filesys.c
    grabdesktop.m
    interaction.m
    main.h
    main.m
    objc_main.h
    video.m
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

set(APPLE_ICON utox.icns)
add_custom_command(OUTPUT utox.icns
    COMMAND iconutil --convert icns utox.iconset -o utox.icns
    DEPENDS utox.iconset
	)
