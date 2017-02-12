project(utoxAV LANGUAGES C)

add_library(utoxAV STATIC
    utox_av.c
    audio.c
    video.c
    )

if(NOT APPLE)
if(WIN32)
    target_link_libraries(utoxAV OpenAL32)
else()
    target_link_libraries(utoxAV openal)
endif()
endif()

if(FILTER_AUDIO)
    target_link_libraries(utoxAV filteraudio)
endif()
