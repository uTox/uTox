cmake_minimum_required(VERSION 3.2)
project(uTox LANGUAGES C)

# This version is for the entire project. All libraries (core, av, ...) move in
# versions in a synchronised way.
set(PROJECT_VERSION_MAJOR "0")
set(PROJECT_VERSION_MINOR "11")
set(PROJECT_VERSION_PATCH "1")
set(PROJECT_VERSION
  "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fno-strict-aliasing")


if(TOXCORE_STATIC)
    # The static windows libs need all of these... because REASONS!
    set(TOX_LIBS
        toxencryptsave
        toxav
        toxdns
        toxcore
        toxgroup
        toxmessenger
        toxfriends
        toxdht
        toxnetcrypto
        toxcrypto
        toxnetwork
        )
else()
    set(TOX_LIBS
        toxencryptsave
        toxdns
        toxav
        toxcore
        )
endif()

add_subdirectory(src)
