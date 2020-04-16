project(qrcodegen)

add_library(${PROJECT_NAME} STATIC
    qrcodegen/c/qrcodegen.c
)

target_include_directories(${PROJECT_NAME}
    SYSTEM PUBLIC qrcodegen/c
)
