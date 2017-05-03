project(utoxAV LANGUAGES C)

add_library(utoxAV STATIC
    utox_av.c
    audio.c
    video.c
    )

if(WIN32)
    target_link_libraries(utoxAV OpenAL32) # Windows needs to be linked against OpenAL32
elseif(APPLE)
    find_package(libopus REQUIRED)
    include_directories(${LIBOPUS_INCLUDE_DIRS})
    target_link_libraries(utoxAV ${LIBOPUS_LIBRARIES}) # Link OSX against opus
else()
    # Both openal and opus are required for the static Linux builds on Jenkins.
    find_package(libopus REQUIRED)
    include_directories(${LIBOPUS_INCLUDE_DIRS})
    target_link_libraries(utoxAV ${LIBOPUS_LIBRARIES})
    target_link_libraries(utoxAV openal)
endif()

if(ENABLE_FILTERAUDIO)
    find_package(libfilteraudio REQUIRED)
    include_directories(${LIBFILTERAUDIO_INCLUDE_DIRS})
    target_link_libraries(utoxAV ${LIBFILTERAUDIO_LIBRARIES})
endif()
