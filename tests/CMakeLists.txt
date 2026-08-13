project(utoxTESTS LANGUAGES C)
include(CTest)

function(make_test name)
    add_executable(test_${name} test_${name}.c)
    set_target_properties(test_${name} PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
    target_link_libraries(test_${name} utox-test-mock pthread)
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

# Settings INI test links production settings.c (via #include) and must not
# pull mock_settings (which stubs config_load/save and owns SETTINGS settings).
add_library(utox-test-mock-no-settings
            mock/mock_filesys.c
            mock/mock_logging.c
            mock/mock_threads.c
            mock/mock_system_calls.c
)
set_target_properties(utox-test-mock-no-settings PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
target_link_libraries(utox-test-mock-no-settings pthread)

# Stubs for domain tests that #include friend/groups/messages/commands/qr.
add_library(utox-test-mock-domain mock/mock_domain.c)
set_target_properties(utox-test-mock-domain PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
target_link_libraries(utox-test-mock-domain utox-test-mock pthread)

# flist_* stubs for tests that do not #include flist.c (test_flist provides the real ones).
add_library(utox-test-mock-flist mock/mock_flist.c)
set_target_properties(utox-test-mock-flist PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
target_link_libraries(utox-test-mock-flist utox-test-mock-domain)

find_package(OpenAL QUIET)
if(OPENAL_INCLUDE_DIR)
    include_directories(${OPENAL_INCLUDE_DIR})
endif()

#
# tests
#
make_test(chatlog)
make_test(chrono)
make_test(messages_chatlog)
make_test(text)
make_test(scrollable)
make_test(theme_parse)
make_test(filesys)

add_executable(test_settings_ini test_settings_ini.c)
set_target_properties(test_settings_ini PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
target_link_libraries(test_settings_ini utox-test-mock-no-settings minini pthread)
add_test(NAME test_settings_ini COMMAND test_settings_ini)

function(make_domain_test name)
    add_executable(test_${name} test_${name}.c)
    set_target_properties(test_${name} PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
    target_link_libraries(test_${name} utox-test-mock-flist utox-test-mock-domain utox-test-mock pthread)
    add_test(NAME test_${name} COMMAND test_${name})
endfunction()

make_domain_test(friend)
make_domain_test(groups)
make_domain_test(messages)
make_domain_test(commands)
make_domain_test(file_transfers)

add_executable(test_flist test_flist.c)
set_target_properties(test_flist PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
target_link_libraries(test_flist utox-test-mock-domain utox-test-mock pthread)
add_test(NAME test_flist COMMAND test_flist)

# avatar.c defines avatar_init; mock_domain already stubs it for friend tests.
add_executable(test_avatar test_avatar.c)
set_target_properties(test_avatar PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
target_link_libraries(test_avatar utox-test-mock pthread)
add_test(NAME test_avatar COMMAND test_avatar)

add_executable(test_self test_self.c)
set_target_properties(test_self PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
target_link_libraries(test_self utox-test-mock pthread)
add_test(NAME test_self COMMAND test_self)

add_executable(test_qr test_qr.c)
set_target_properties(test_qr PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
target_link_libraries(test_qr utox-test-mock-flist utox-test-mock-domain utox-test-mock qrcodegen stb pthread m)
add_test(NAME test_qr COMMAND test_qr)
