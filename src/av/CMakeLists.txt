project(utoxAV LANGUAGES C)

add_library(utoxAV STATIC
    utox_av.c
    audio.c
    video.c
    )

if(WIN32)
    target_link_libraries(utoxAV OpenAL32)
elseif(NOT APPLE)
    target_link_libraries(utoxAV openal)
else()
    target_link_libraries(utoxAV opus)
endif()

if(ENABLE_FILTERAUDIO)
    target_link_libraries(utoxAV filteraudio)
endif()
