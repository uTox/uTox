project(minIni)

include(FetchContent)

FetchContent_Declare(minIni
    GIT_REPOSITORY https://github.com/compuphase/minIni.git
    GIT_TAG 77908366d4c0a758e33c4c6f39a29eba836ae7dd
)

FetchContent_GetProperties(minIni)
if(NOT minIni_POPULATED)
    FetchContent_Populate(minIni)
endif()

FetchContent_GetProperties(minIni
    SOURCE_DIR minIni_SOURCE_DIR
)

add_library(${PROJECT_NAME}
    ${minIni_SOURCE_DIR}/dev/minIni.c
    ${minIni_SOURCE_DIR}/dev/minIni.h
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${minIni_SOURCE_DIR}/dev
)
