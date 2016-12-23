cmake_minimum_required(VERSION 3.2)
project(utoxAV LANGUAGES C)

if(UNIX)
    # this is needed because we include main_native.h from audio.c, which depends on freetype
    # TODO could be removed if we manage to remove that dependency
    include_directories(/usr/include/freetype2)
endif()

add_library(utoxAV STATIC
    utox_av.h
    utox_av.c
    audio.c
    audio.h
    video.h
    video.c
    )

if(NOT APPLE)
    target_link_libraries(utoxAV openal)
endif()

if(FILTER_AUDIO)
    target_link_libraries(utoxAV filteraudio)
endif()
