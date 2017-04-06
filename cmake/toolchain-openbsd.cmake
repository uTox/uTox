# Required to prevent duplication of flags from this file.
UNSET(CMAKE_C_FLAGS CACHE)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I/usr/local/include -L/usr/local/lib -I/usr/X11R6/include -L/usr/X11R6/lib -std=c99" CACHE STRING "" FORCE)

set(ENABLE_TESTS OFF CACHE STRING "" FORCE)
set(FILTER_AUDIO OFF CACHE STRING "" FORCE)

set(LIBRARIES
    opus
	openal
	v4lconvert
	CACHE STRING "" FORCE)
