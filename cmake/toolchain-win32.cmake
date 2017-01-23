# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)

set(COMPILER_PREFIX "i686-w64-mingw32")
set(CMAKE_C_COMPILER   i686-w64-mingw32-gcc )
set(CMAKE_RC_COMPILER  i686-w64-mingw32-windres )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -DAL_LIBTYPE_STATIC")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static -O3 -s -std=gnu99 -w -DAL_LIBTYPE_STATIC")

set(INCLUDE_DIRECTORIES SYSTEM /usr/share/mingw-w64/include/)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(WIN32 TRUE)
set(UNIX FALSE)

set(ENABLE_ASAN OFF)
