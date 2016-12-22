cmake_minimum_required(VERSION 3.2)
project(uTox LANGUAGES C)

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
