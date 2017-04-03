# This file prevents an in-source-tree build which would mix cloned files and
# build files.
#
# Now you have to do at least:
#
# $ mkdir build
# $ cd build
# $ cmake ../
#
# But I recommend something like shown in error message below.

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "CMake has been ran to create an in-source build. Please create a 'build' path like following and run cmake in there:

$ mkdir -p build/debug
$ cd build/debug
$ cmake ../.. -DCMAKE_BUILD_TYPE=Debug

This allows you to have multiple builds (e.g. debug and release) from same source code tree and no build files are being mixed with source files.
And please cleanup CMakeFiles/ and CMakeCache.txt, this cannot be done from a cmake file:
https://cmake.org/pipermail/cmake/2014-January/056789.html")
endif ()
