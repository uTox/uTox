project(utoxTESTS LANGUAGES C)
include(CTest)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(Check REQUIRED)

function(make_test name)
    add_executable(test_${name} test_${name}.c)
    set_target_properties(test_${name} PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
    target_link_libraries(test_${name} utox-test-mock ${CHECK_LIBRARIES})
    add_test(NAME test_${name} COMMAND test_${name})
endfunction()

configure_file(${utoxTESTS_SOURCE_DIR}/run_tests.sh
               ${uTox_BINARY_DIR}/run_tests.sh)

#
# generic testing defines
#
include_directories("../src")

add_library(utox-test-mock
            mock/mock_filesys.c
            mock/mock_settings.c
            mock/mock_logging.c
            mock/mock_threads.c
            mock/mock_system_calls.c
)
set_target_properties(utox-test-mock PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
target_link_libraries(utox-test-mock pthread)

#
# tests
#
# TODO add a cmake macro for adding tests, this will be too verbose if we add more.
make_test(chatlog)

if(ENABLE_AUTOUPDATE)
  make_test(updater)
      target_link_libraries(test_updater sodium)
endif()
