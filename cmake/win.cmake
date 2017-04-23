# Required to prevent duplication of flags from this file.
UNSET(CMAKE_C_FLAGS CACHE)
UNSET(CMAKE_C_FLAGS_DEBUG CACHE)
UNSET(CMAKE_C_FLAGS_RELWITHDEBINFO CACHE)

# Windows only compiles statically.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DAL_LIBTYPE_STATIC")
set(UTOX_STATIC ON)
set(TOXCORE_STATIC ON)

# ASAN is incompatible with static compilation.
if (ENABLE_ASAN)
    message(WARNING "ASAN is incompatible with static compilation and will be disabled.")
    set(ENABLE_ASAN OFF)
endif()

# Required for line numbers in gdb on Windows.
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g3" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -g3" CACHE STRING "" FORCE)
