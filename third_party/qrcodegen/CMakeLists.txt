project(qrcodegen)

add_library(${PROJECT_NAME}
    qrcodegen/c/qrcodegen.c
)

target_include_directories(${PROJECT_NAME}
    SYSTEM PUBLIC qrcodegen/c
)
