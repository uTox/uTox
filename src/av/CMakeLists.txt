project(utoxAV LANGUAGES C)

add_library(utoxAV STATIC
    utox_av.c
    audio.c
    video.c
    )

if(WIN32)
    target_link_libraries(utoxAV OpenAL32) # Windows needs to be linked against OpenAL32
elseif(APPLE)
    target_link_libraries(utoxAV opus) # Link OSX against opus
else()
    # Both openal and opus are required for the static Linux builds on Jenkins.
    target_link_libraries(utoxAV openal)
    target_link_libraries(utoxAV opus)
endif()

if(ENABLE_FILTERAUDIO)
    target_link_libraries(utoxAV filteraudio)
endif()
