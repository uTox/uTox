project(utoxAV LANGUAGES C)

add_library(utoxAV STATIC
    utox_av.c
    audio.c
    video.c
    )

if(WIN32)
    target_link_libraries(utoxAV OpenAL32) # Windows needs to be linked against OpenAL32
elseif(NOT APPLE)
    target_link_libraries(utoxAV openal) #Everything but OSX can be linked against openal
else()
    target_link_libraries(utoxAV opus) # Link OSX against opus
endif()

if(ENABLE_FILTERAUDIO)
    target_link_libraries(utoxAV filteraudio)
endif()
