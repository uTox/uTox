project(stb)

add_library(${PROJECT_NAME} STATIC
    stb.c
)

target_include_directories(${PROJECT_NAME}
    SYSTEM PUBLIC .
)
