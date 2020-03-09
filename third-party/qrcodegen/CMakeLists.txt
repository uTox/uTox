project(qrcodegen)

include(FetchContent)

FetchContent_Declare(qrcodegen
    GIT_REPOSITORY https://github.com/nayuki/QR-Code-generator.git
    GIT_TAG 7e18dfe885f1cf2924078542927496c35d575ce7
)

FetchContent_GetProperties(qrcodegen)
if(NOT qrcodegen_POPULATED)
    FetchContent_Populate(qrcodegen)
endif()

FetchContent_GetProperties(qrcodegen
    SOURCE_DIR qrcodegen_SOURCE_DIR
)

add_library(${PROJECT_NAME}
    ${qrcodegen_SOURCE_DIR}/c/qrcodegen.c
    ${qrcodegen_SOURCE_DIR}/c/qrcodegen.h
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${qrcodegen_SOURCE_DIR}/c
)
