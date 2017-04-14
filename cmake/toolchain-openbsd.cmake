# Required to prevent duplication of flags from this file.
UNSET(CMAKE_C_FLAGS CACHE)

# I think this is correct and will work on all systems but I could be wrong
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I/usr/local/include -L/usr/local/lib -I/usr/X11R6/include -L/usr/X11R6/lib -std=c99" CACHE STRING "" FORCE)

set(ENABLE_TESTS OFF CACHE STRING "" FORCE)
set(ENABLE_FILTERAUDIO OFF CACHE STRING "" FORCE)
set(ENABLE_DBUS OFF CACHE STRING "" FORCE)
