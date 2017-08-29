# Having a mingw32 compiler set is vital. Otherwise you get weird errors because
# of Windows headers.
# From $UTOX_ROOT/build/
# CC=x86_64-w64-mingw32-gcc cmake .. -DCMAKE_BUILD_TYPE=Debug
# make -j

# Required to prevent duplication of flags from this file.
UNSET(CMAKE_C_FLAGS CACHE)
UNSET(CMAKE_C_FLAGS_DEBUG CACHE)
UNSET(CMAKE_C_FLAGS_RELWITHDEBINFO CACHE)

# Windows only compiles statically.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DAL_LIBTYPE_STATIC")

# Required for line numbers in gdb on Windows.
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g3" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -g3" CACHE STRING "" FORCE)

# Set default dependency path.
if(NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/libs/windows-x64)
endif()
