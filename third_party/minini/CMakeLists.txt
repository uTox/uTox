project(minini)

add_library(${PROJECT_NAME} STATIC
    minini/dev/minIni.c
)

target_include_directories(${PROJECT_NAME}
    SYSTEM PUBLIC minini/dev
)
