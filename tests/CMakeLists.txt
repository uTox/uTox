project(utoxTESTS LANGUAGES C)

#
# generic testing defines
#

include_directories("../src")

add_library(utox-test-mock

    mock/mock_filesys.c
    mock/mock_settings.c
    mock/mock_logging.c
)
set_target_properties(utox-test-mock PROPERTIES
    COMPILE_FLAGS "-Wno-unused-parameter"
)
configure_file(${utoxTESTS_SOURCE_DIR}/run_tests.sh ${uTox_BINARY_DIR}/run_tests.sh)

#
# tests
#

# TODO add a cmake macro for adding tests, this will be too verbose if we add more.
add_executable(test_chatlog
    test_chatlog.c

    ../src/chatlog.c
    ../src/text.c
)
set_target_properties(test_chatlog PROPERTIES
    COMPILE_FLAGS "-Wno-unused-parameter"
)
target_link_libraries(test_chatlog utox-test-mock)
add_test(NAME test_chatlog COMMAND test_chatlog)

