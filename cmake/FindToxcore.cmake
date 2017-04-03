# Locate Toxcore
#
# This module defines:
#
# TOXCORE_LIBRARIES
# TOXCORE_FOUND, if false, fail because Toxcoreis missing
# TOXCORE_INCLUDE_DIR, where to find the headers
#
# $TOXDIR is an environment variable building Toxcore.
#
# Initially created by Erik Hofman for SimGear project
#
# Cleaned up for utox

find_path(TOXCORE_INCLUDE_DIR tox/tox.h
    HINTS
    $ENV{TOXCOREDIR}
    PATH_SUFFIXES include
    PATHS
    /usr/local
    /usr
    /opt
)

find_library(TOXCORE_LIBRARY
    NAMES libtoxcore toxcore libtoxcore32 toxcore32
    HINTS
    $ENV{TOXCOREDIR}
    PATH_SUFFIXES bin lib lib/${CMAKE_LIBRARY_ARCHITECTURE} lib64 libs64 libs libs/Win32 libs/Win64
    PATHS
    /usr
    /opt
    /usr/local
)

set(TOXCORE_FOUND "NO")
if(TOXCORE_LIBRARY AND TOXCORE_INCLUDE_DIR)
    set(TOXCORE_FOUND "YES")
else()
    message(WARNING "Cannot find Toxcore, setup may fail.")
endif()
