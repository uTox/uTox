project(stb)

include(FetchContent)

FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 9d9f75eb682dd98b34de08bb5c489c6c561c9fa6
)

FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
endif()

FetchContent_GetProperties(stb
    SOURCE_DIR stb_SOURCE_DIR
)

add_library(${PROJECT_NAME}
    ${stb_SOURCE_DIR}/stb_image.h
    ${stb_SOURCE_DIR}/stb_image_write.h
    stb.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${stb_SOURCE_DIR}
        .
)
