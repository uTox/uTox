project(utoxAV LANGUAGES C)

add_library(utoxAV STATIC
    utox_av.c
    audio.c
    video.c
    filter_audio.c
    )

if(APPLE)
    find_package(libopus REQUIRED)
    include_directories(${LIBOPUS_INCLUDE_DIRS})
    target_link_libraries(utoxAV ${LIBOPUS_LIBRARIES}) # Link OSX against opus
else()
    # Linux and Windows: locate OpenAL via CMake
    if(NOT WIN32)
        # Opus is system/pkg-config on Unix; Windows pulls it in via other deps.
        find_package(libopus REQUIRED)
        include_directories(${LIBOPUS_INCLUDE_DIRS})
        target_link_libraries(utoxAV ${LIBOPUS_LIBRARIES})
    endif()
    find_package(OpenAL REQUIRED)
    include_directories(${OPENAL_INCLUDE_DIR})
    target_link_libraries(utoxAV ${OPENAL_LIBRARY})
endif()

if(ENABLE_FILTERAUDIO)
    find_package(libfilteraudio REQUIRED)
    include_directories(${LIBFILTERAUDIO_INCLUDE_DIRS})
    target_link_libraries(utoxAV ${LIBFILTERAUDIO_LIBRARIES})
endif()
