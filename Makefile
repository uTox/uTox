# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.6

# Default target executed when no arguments are given to make.
default_target: all

.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Produce verbose output by default.
VERBOSE = 1

# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/grayhatter/code/utox

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/grayhatter/code/utox

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake cache editor..."
	/usr/bin/ccmake -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache

.PHONY : edit_cache/fast

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/usr/bin/cmake -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache

.PHONY : rebuild_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/grayhatter/code/utox/CMakeFiles /home/grayhatter/code/utox/CMakeFiles/progress.marks
	$(MAKE) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/grayhatter/code/utox/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean

.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named icon

# Build rule for target.
icon: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 icon
.PHONY : icon

# fast build rule for target.
icon/fast:
	$(MAKE) -f CMakeFiles/icon.dir/build.make CMakeFiles/icon.dir/build
.PHONY : icon/fast

#=============================================================================
# Target rules for targets named utox

# Build rule for target.
utox: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 utox
.PHONY : utox

# fast build rule for target.
utox/fast:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/build
.PHONY : utox/fast

src/av/audio.o: src/av/audio.c.o

.PHONY : src/av/audio.o

# target to build an object file
src/av/audio.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/av/audio.c.o
.PHONY : src/av/audio.c.o

src/av/audio.i: src/av/audio.c.i

.PHONY : src/av/audio.i

# target to preprocess a source file
src/av/audio.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/av/audio.c.i
.PHONY : src/av/audio.c.i

src/av/audio.s: src/av/audio.c.s

.PHONY : src/av/audio.s

# target to generate assembly for a file
src/av/audio.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/av/audio.c.s
.PHONY : src/av/audio.c.s

src/av/utox_av.o: src/av/utox_av.c.o

.PHONY : src/av/utox_av.o

# target to build an object file
src/av/utox_av.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/av/utox_av.c.o
.PHONY : src/av/utox_av.c.o

src/av/utox_av.i: src/av/utox_av.c.i

.PHONY : src/av/utox_av.i

# target to preprocess a source file
src/av/utox_av.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/av/utox_av.c.i
.PHONY : src/av/utox_av.c.i

src/av/utox_av.s: src/av/utox_av.c.s

.PHONY : src/av/utox_av.s

# target to generate assembly for a file
src/av/utox_av.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/av/utox_av.c.s
.PHONY : src/av/utox_av.c.s

src/av/video.o: src/av/video.c.o

.PHONY : src/av/video.o

# target to build an object file
src/av/video.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/av/video.c.o
.PHONY : src/av/video.c.o

src/av/video.i: src/av/video.c.i

.PHONY : src/av/video.i

# target to preprocess a source file
src/av/video.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/av/video.c.i
.PHONY : src/av/video.c.i

src/av/video.s: src/av/video.c.s

.PHONY : src/av/video.s

# target to generate assembly for a file
src/av/video.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/av/video.c.s
.PHONY : src/av/video.c.s

src/avatar.o: src/avatar.c.o

.PHONY : src/avatar.o

# target to build an object file
src/avatar.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/avatar.c.o
.PHONY : src/avatar.c.o

src/avatar.i: src/avatar.c.i

.PHONY : src/avatar.i

# target to preprocess a source file
src/avatar.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/avatar.c.i
.PHONY : src/avatar.c.i

src/avatar.s: src/avatar.c.s

.PHONY : src/avatar.s

# target to generate assembly for a file
src/avatar.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/avatar.c.s
.PHONY : src/avatar.c.s

src/commands.o: src/commands.c.o

.PHONY : src/commands.o

# target to build an object file
src/commands.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/commands.c.o
.PHONY : src/commands.c.o

src/commands.i: src/commands.c.i

.PHONY : src/commands.i

# target to preprocess a source file
src/commands.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/commands.c.i
.PHONY : src/commands.c.i

src/commands.s: src/commands.c.s

.PHONY : src/commands.s

# target to generate assembly for a file
src/commands.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/commands.c.s
.PHONY : src/commands.c.s

src/devices.o: src/devices.c.o

.PHONY : src/devices.o

# target to build an object file
src/devices.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/devices.c.o
.PHONY : src/devices.c.o

src/devices.i: src/devices.c.i

.PHONY : src/devices.i

# target to preprocess a source file
src/devices.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/devices.c.i
.PHONY : src/devices.c.i

src/devices.s: src/devices.c.s

.PHONY : src/devices.s

# target to generate assembly for a file
src/devices.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/devices.c.s
.PHONY : src/devices.c.s

src/dns.o: src/dns.c.o

.PHONY : src/dns.o

# target to build an object file
src/dns.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/dns.c.o
.PHONY : src/dns.c.o

src/dns.i: src/dns.c.i

.PHONY : src/dns.i

# target to preprocess a source file
src/dns.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/dns.c.i
.PHONY : src/dns.c.i

src/dns.s: src/dns.c.s

.PHONY : src/dns.s

# target to generate assembly for a file
src/dns.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/dns.c.s
.PHONY : src/dns.c.s

src/file_transfers.o: src/file_transfers.c.o

.PHONY : src/file_transfers.o

# target to build an object file
src/file_transfers.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/file_transfers.c.o
.PHONY : src/file_transfers.c.o

src/file_transfers.i: src/file_transfers.c.i

.PHONY : src/file_transfers.i

# target to preprocess a source file
src/file_transfers.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/file_transfers.c.i
.PHONY : src/file_transfers.c.i

src/file_transfers.s: src/file_transfers.c.s

.PHONY : src/file_transfers.s

# target to generate assembly for a file
src/file_transfers.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/file_transfers.c.s
.PHONY : src/file_transfers.c.s

src/flist.o: src/flist.c.o

.PHONY : src/flist.o

# target to build an object file
src/flist.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/flist.c.o
.PHONY : src/flist.c.o

src/flist.i: src/flist.c.i

.PHONY : src/flist.i

# target to preprocess a source file
src/flist.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/flist.c.i
.PHONY : src/flist.c.i

src/flist.s: src/flist.c.s

.PHONY : src/flist.s

# target to generate assembly for a file
src/flist.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/flist.c.s
.PHONY : src/flist.c.s

src/friend.o: src/friend.c.o

.PHONY : src/friend.o

# target to build an object file
src/friend.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/friend.c.o
.PHONY : src/friend.c.o

src/friend.i: src/friend.c.i

.PHONY : src/friend.i

# target to preprocess a source file
src/friend.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/friend.c.i
.PHONY : src/friend.c.i

src/friend.s: src/friend.c.s

.PHONY : src/friend.s

# target to generate assembly for a file
src/friend.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/friend.c.s
.PHONY : src/friend.c.s

src/groups.o: src/groups.c.o

.PHONY : src/groups.o

# target to build an object file
src/groups.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/groups.c.o
.PHONY : src/groups.c.o

src/groups.i: src/groups.c.i

.PHONY : src/groups.i

# target to preprocess a source file
src/groups.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/groups.c.i
.PHONY : src/groups.c.i

src/groups.s: src/groups.c.s

.PHONY : src/groups.s

# target to generate assembly for a file
src/groups.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/groups.c.s
.PHONY : src/groups.c.s

src/inline_video.o: src/inline_video.c.o

.PHONY : src/inline_video.o

# target to build an object file
src/inline_video.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/inline_video.c.o
.PHONY : src/inline_video.c.o

src/inline_video.i: src/inline_video.c.i

.PHONY : src/inline_video.i

# target to preprocess a source file
src/inline_video.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/inline_video.c.i
.PHONY : src/inline_video.c.i

src/inline_video.s: src/inline_video.c.s

.PHONY : src/inline_video.s

# target to generate assembly for a file
src/inline_video.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/inline_video.c.s
.PHONY : src/inline_video.c.s

src/main.o: src/main.c.o

.PHONY : src/main.o

# target to build an object file
src/main.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/main.c.o
.PHONY : src/main.c.o

src/main.i: src/main.c.i

.PHONY : src/main.i

# target to preprocess a source file
src/main.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/main.c.i
.PHONY : src/main.c.i

src/main.s: src/main.c.s

.PHONY : src/main.s

# target to generate assembly for a file
src/main.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/main.c.s
.PHONY : src/main.c.s

src/messages.o: src/messages.c.o

.PHONY : src/messages.o

# target to build an object file
src/messages.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/messages.c.o
.PHONY : src/messages.c.o

src/messages.i: src/messages.c.i

.PHONY : src/messages.i

# target to preprocess a source file
src/messages.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/messages.c.i
.PHONY : src/messages.c.i

src/messages.s: src/messages.c.s

.PHONY : src/messages.s

# target to generate assembly for a file
src/messages.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/messages.c.s
.PHONY : src/messages.c.s

src/theme.o: src/theme.c.o

.PHONY : src/theme.o

# target to build an object file
src/theme.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/theme.c.o
.PHONY : src/theme.c.o

src/theme.i: src/theme.c.i

.PHONY : src/theme.i

# target to preprocess a source file
src/theme.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/theme.c.i
.PHONY : src/theme.c.i

src/theme.s: src/theme.c.s

.PHONY : src/theme.s

# target to generate assembly for a file
src/theme.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/theme.c.s
.PHONY : src/theme.c.s

src/tox.o: src/tox.c.o

.PHONY : src/tox.o

# target to build an object file
src/tox.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/tox.c.o
.PHONY : src/tox.c.o

src/tox.i: src/tox.c.i

.PHONY : src/tox.i

# target to preprocess a source file
src/tox.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/tox.c.i
.PHONY : src/tox.c.i

src/tox.s: src/tox.c.s

.PHONY : src/tox.s

# target to generate assembly for a file
src/tox.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/tox.c.s
.PHONY : src/tox.c.s

src/tox_callbacks.o: src/tox_callbacks.c.o

.PHONY : src/tox_callbacks.o

# target to build an object file
src/tox_callbacks.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/tox_callbacks.c.o
.PHONY : src/tox_callbacks.c.o

src/tox_callbacks.i: src/tox_callbacks.c.i

.PHONY : src/tox_callbacks.i

# target to preprocess a source file
src/tox_callbacks.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/tox_callbacks.c.i
.PHONY : src/tox_callbacks.c.i

src/tox_callbacks.s: src/tox_callbacks.c.s

.PHONY : src/tox_callbacks.s

# target to generate assembly for a file
src/tox_callbacks.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/tox_callbacks.c.s
.PHONY : src/tox_callbacks.c.s

src/ui.o: src/ui.c.o

.PHONY : src/ui.o

# target to build an object file
src/ui.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui.c.o
.PHONY : src/ui.c.o

src/ui.i: src/ui.c.i

.PHONY : src/ui.i

# target to preprocess a source file
src/ui.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui.c.i
.PHONY : src/ui.c.i

src/ui.s: src/ui.c.s

.PHONY : src/ui.s

# target to generate assembly for a file
src/ui.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui.c.s
.PHONY : src/ui.c.s

src/ui/button.o: src/ui/button.c.o

.PHONY : src/ui/button.o

# target to build an object file
src/ui/button.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/button.c.o
.PHONY : src/ui/button.c.o

src/ui/button.i: src/ui/button.c.i

.PHONY : src/ui/button.i

# target to preprocess a source file
src/ui/button.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/button.c.i
.PHONY : src/ui/button.c.i

src/ui/button.s: src/ui/button.c.s

.PHONY : src/ui/button.s

# target to generate assembly for a file
src/ui/button.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/button.c.s
.PHONY : src/ui/button.c.s

src/ui/buttons.o: src/ui/buttons.c.o

.PHONY : src/ui/buttons.o

# target to build an object file
src/ui/buttons.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/buttons.c.o
.PHONY : src/ui/buttons.c.o

src/ui/buttons.i: src/ui/buttons.c.i

.PHONY : src/ui/buttons.i

# target to preprocess a source file
src/ui/buttons.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/buttons.c.i
.PHONY : src/ui/buttons.c.i

src/ui/buttons.s: src/ui/buttons.c.s

.PHONY : src/ui/buttons.s

# target to generate assembly for a file
src/ui/buttons.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/buttons.c.s
.PHONY : src/ui/buttons.c.s

src/ui/contextmenu.o: src/ui/contextmenu.c.o

.PHONY : src/ui/contextmenu.o

# target to build an object file
src/ui/contextmenu.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/contextmenu.c.o
.PHONY : src/ui/contextmenu.c.o

src/ui/contextmenu.i: src/ui/contextmenu.c.i

.PHONY : src/ui/contextmenu.i

# target to preprocess a source file
src/ui/contextmenu.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/contextmenu.c.i
.PHONY : src/ui/contextmenu.c.i

src/ui/contextmenu.s: src/ui/contextmenu.c.s

.PHONY : src/ui/contextmenu.s

# target to generate assembly for a file
src/ui/contextmenu.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/contextmenu.c.s
.PHONY : src/ui/contextmenu.c.s

src/ui/draw_helpers.o: src/ui/draw_helpers.c.o

.PHONY : src/ui/draw_helpers.o

# target to build an object file
src/ui/draw_helpers.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/draw_helpers.c.o
.PHONY : src/ui/draw_helpers.c.o

src/ui/draw_helpers.i: src/ui/draw_helpers.c.i

.PHONY : src/ui/draw_helpers.i

# target to preprocess a source file
src/ui/draw_helpers.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/draw_helpers.c.i
.PHONY : src/ui/draw_helpers.c.i

src/ui/draw_helpers.s: src/ui/draw_helpers.c.s

.PHONY : src/ui/draw_helpers.s

# target to generate assembly for a file
src/ui/draw_helpers.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/draw_helpers.c.s
.PHONY : src/ui/draw_helpers.c.s

src/ui/dropdown.o: src/ui/dropdown.c.o

.PHONY : src/ui/dropdown.o

# target to build an object file
src/ui/dropdown.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/dropdown.c.o
.PHONY : src/ui/dropdown.c.o

src/ui/dropdown.i: src/ui/dropdown.c.i

.PHONY : src/ui/dropdown.i

# target to preprocess a source file
src/ui/dropdown.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/dropdown.c.i
.PHONY : src/ui/dropdown.c.i

src/ui/dropdown.s: src/ui/dropdown.c.s

.PHONY : src/ui/dropdown.s

# target to generate assembly for a file
src/ui/dropdown.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/dropdown.c.s
.PHONY : src/ui/dropdown.c.s

src/ui/dropdowns.o: src/ui/dropdowns.c.o

.PHONY : src/ui/dropdowns.o

# target to build an object file
src/ui/dropdowns.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/dropdowns.c.o
.PHONY : src/ui/dropdowns.c.o

src/ui/dropdowns.i: src/ui/dropdowns.c.i

.PHONY : src/ui/dropdowns.i

# target to preprocess a source file
src/ui/dropdowns.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/dropdowns.c.i
.PHONY : src/ui/dropdowns.c.i

src/ui/dropdowns.s: src/ui/dropdowns.c.s

.PHONY : src/ui/dropdowns.s

# target to generate assembly for a file
src/ui/dropdowns.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/dropdowns.c.s
.PHONY : src/ui/dropdowns.c.s

src/ui/edit.o: src/ui/edit.c.o

.PHONY : src/ui/edit.o

# target to build an object file
src/ui/edit.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/edit.c.o
.PHONY : src/ui/edit.c.o

src/ui/edit.i: src/ui/edit.c.i

.PHONY : src/ui/edit.i

# target to preprocess a source file
src/ui/edit.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/edit.c.i
.PHONY : src/ui/edit.c.i

src/ui/edit.s: src/ui/edit.c.s

.PHONY : src/ui/edit.s

# target to generate assembly for a file
src/ui/edit.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/edit.c.s
.PHONY : src/ui/edit.c.s

src/ui/edits.o: src/ui/edits.c.o

.PHONY : src/ui/edits.o

# target to build an object file
src/ui/edits.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/edits.c.o
.PHONY : src/ui/edits.c.o

src/ui/edits.i: src/ui/edits.c.i

.PHONY : src/ui/edits.i

# target to preprocess a source file
src/ui/edits.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/edits.c.i
.PHONY : src/ui/edits.c.i

src/ui/edits.s: src/ui/edits.c.s

.PHONY : src/ui/edits.s

# target to generate assembly for a file
src/ui/edits.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/edits.c.s
.PHONY : src/ui/edits.c.s

src/ui/scrollable.o: src/ui/scrollable.c.o

.PHONY : src/ui/scrollable.o

# target to build an object file
src/ui/scrollable.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/scrollable.c.o
.PHONY : src/ui/scrollable.c.o

src/ui/scrollable.i: src/ui/scrollable.c.i

.PHONY : src/ui/scrollable.i

# target to preprocess a source file
src/ui/scrollable.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/scrollable.c.i
.PHONY : src/ui/scrollable.c.i

src/ui/scrollable.s: src/ui/scrollable.c.s

.PHONY : src/ui/scrollable.s

# target to generate assembly for a file
src/ui/scrollable.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/scrollable.c.s
.PHONY : src/ui/scrollable.c.s

src/ui/svg.o: src/ui/svg.c.o

.PHONY : src/ui/svg.o

# target to build an object file
src/ui/svg.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/svg.c.o
.PHONY : src/ui/svg.c.o

src/ui/svg.i: src/ui/svg.c.i

.PHONY : src/ui/svg.i

# target to preprocess a source file
src/ui/svg.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/svg.c.i
.PHONY : src/ui/svg.c.i

src/ui/svg.s: src/ui/svg.c.s

.PHONY : src/ui/svg.s

# target to generate assembly for a file
src/ui/svg.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/svg.c.s
.PHONY : src/ui/svg.c.s

src/ui/switch.o: src/ui/switch.c.o

.PHONY : src/ui/switch.o

# target to build an object file
src/ui/switch.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/switch.c.o
.PHONY : src/ui/switch.c.o

src/ui/switch.i: src/ui/switch.c.i

.PHONY : src/ui/switch.i

# target to preprocess a source file
src/ui/switch.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/switch.c.i
.PHONY : src/ui/switch.c.i

src/ui/switch.s: src/ui/switch.c.s

.PHONY : src/ui/switch.s

# target to generate assembly for a file
src/ui/switch.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/switch.c.s
.PHONY : src/ui/switch.c.s

src/ui/switches.o: src/ui/switches.c.o

.PHONY : src/ui/switches.o

# target to build an object file
src/ui/switches.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/switches.c.o
.PHONY : src/ui/switches.c.o

src/ui/switches.i: src/ui/switches.c.i

.PHONY : src/ui/switches.i

# target to preprocess a source file
src/ui/switches.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/switches.c.i
.PHONY : src/ui/switches.c.i

src/ui/switches.s: src/ui/switches.c.s

.PHONY : src/ui/switches.s

# target to generate assembly for a file
src/ui/switches.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/switches.c.s
.PHONY : src/ui/switches.c.s

src/ui/text.o: src/ui/text.c.o

.PHONY : src/ui/text.o

# target to build an object file
src/ui/text.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/text.c.o
.PHONY : src/ui/text.c.o

src/ui/text.i: src/ui/text.c.i

.PHONY : src/ui/text.i

# target to preprocess a source file
src/ui/text.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/text.c.i
.PHONY : src/ui/text.c.i

src/ui/text.s: src/ui/text.c.s

.PHONY : src/ui/text.s

# target to generate assembly for a file
src/ui/text.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/text.c.s
.PHONY : src/ui/text.c.s

src/ui/tooltip.o: src/ui/tooltip.c.o

.PHONY : src/ui/tooltip.o

# target to build an object file
src/ui/tooltip.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/tooltip.c.o
.PHONY : src/ui/tooltip.c.o

src/ui/tooltip.i: src/ui/tooltip.c.i

.PHONY : src/ui/tooltip.i

# target to preprocess a source file
src/ui/tooltip.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/tooltip.c.i
.PHONY : src/ui/tooltip.c.i

src/ui/tooltip.s: src/ui/tooltip.c.s

.PHONY : src/ui/tooltip.s

# target to generate assembly for a file
src/ui/tooltip.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui/tooltip.c.s
.PHONY : src/ui/tooltip.c.s

src/ui_i18n.o: src/ui_i18n.c.o

.PHONY : src/ui_i18n.o

# target to build an object file
src/ui_i18n.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui_i18n.c.o
.PHONY : src/ui_i18n.c.o

src/ui_i18n.i: src/ui_i18n.c.i

.PHONY : src/ui_i18n.i

# target to preprocess a source file
src/ui_i18n.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui_i18n.c.i
.PHONY : src/ui_i18n.c.i

src/ui_i18n.s: src/ui_i18n.c.s

.PHONY : src/ui_i18n.s

# target to generate assembly for a file
src/ui_i18n.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/ui_i18n.c.s
.PHONY : src/ui_i18n.c.s

src/util.o: src/util.c.o

.PHONY : src/util.o

# target to build an object file
src/util.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/util.c.o
.PHONY : src/util.c.o

src/util.i: src/util.c.i

.PHONY : src/util.i

# target to preprocess a source file
src/util.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/util.c.i
.PHONY : src/util.c.i

src/util.s: src/util.c.s

.PHONY : src/util.s

# target to generate assembly for a file
src/util.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/util.c.s
.PHONY : src/util.c.s

src/xlib/audio.o: src/xlib/audio.c.o

.PHONY : src/xlib/audio.o

# target to build an object file
src/xlib/audio.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/audio.c.o
.PHONY : src/xlib/audio.c.o

src/xlib/audio.i: src/xlib/audio.c.i

.PHONY : src/xlib/audio.i

# target to preprocess a source file
src/xlib/audio.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/audio.c.i
.PHONY : src/xlib/audio.c.i

src/xlib/audio.s: src/xlib/audio.c.s

.PHONY : src/xlib/audio.s

# target to generate assembly for a file
src/xlib/audio.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/audio.c.s
.PHONY : src/xlib/audio.c.s

src/xlib/dbus.o: src/xlib/dbus.c.o

.PHONY : src/xlib/dbus.o

# target to build an object file
src/xlib/dbus.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/dbus.c.o
.PHONY : src/xlib/dbus.c.o

src/xlib/dbus.i: src/xlib/dbus.c.i

.PHONY : src/xlib/dbus.i

# target to preprocess a source file
src/xlib/dbus.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/dbus.c.i
.PHONY : src/xlib/dbus.c.i

src/xlib/dbus.s: src/xlib/dbus.c.s

.PHONY : src/xlib/dbus.s

# target to generate assembly for a file
src/xlib/dbus.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/dbus.c.s
.PHONY : src/xlib/dbus.c.s

src/xlib/drawing.o: src/xlib/drawing.c.o

.PHONY : src/xlib/drawing.o

# target to build an object file
src/xlib/drawing.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/drawing.c.o
.PHONY : src/xlib/drawing.c.o

src/xlib/drawing.i: src/xlib/drawing.c.i

.PHONY : src/xlib/drawing.i

# target to preprocess a source file
src/xlib/drawing.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/drawing.c.i
.PHONY : src/xlib/drawing.c.i

src/xlib/drawing.s: src/xlib/drawing.c.s

.PHONY : src/xlib/drawing.s

# target to generate assembly for a file
src/xlib/drawing.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/drawing.c.s
.PHONY : src/xlib/drawing.c.s

src/xlib/event.o: src/xlib/event.c.o

.PHONY : src/xlib/event.o

# target to build an object file
src/xlib/event.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/event.c.o
.PHONY : src/xlib/event.c.o

src/xlib/event.i: src/xlib/event.c.i

.PHONY : src/xlib/event.i

# target to preprocess a source file
src/xlib/event.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/event.c.i
.PHONY : src/xlib/event.c.i

src/xlib/event.s: src/xlib/event.c.s

.PHONY : src/xlib/event.s

# target to generate assembly for a file
src/xlib/event.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/event.c.s
.PHONY : src/xlib/event.c.s

src/xlib/freetype.o: src/xlib/freetype.c.o

.PHONY : src/xlib/freetype.o

# target to build an object file
src/xlib/freetype.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/freetype.c.o
.PHONY : src/xlib/freetype.c.o

src/xlib/freetype.i: src/xlib/freetype.c.i

.PHONY : src/xlib/freetype.i

# target to preprocess a source file
src/xlib/freetype.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/freetype.c.i
.PHONY : src/xlib/freetype.c.i

src/xlib/freetype.s: src/xlib/freetype.c.s

.PHONY : src/xlib/freetype.s

# target to generate assembly for a file
src/xlib/freetype.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/freetype.c.s
.PHONY : src/xlib/freetype.c.s

src/xlib/gtk.o: src/xlib/gtk.c.o

.PHONY : src/xlib/gtk.o

# target to build an object file
src/xlib/gtk.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/gtk.c.o
.PHONY : src/xlib/gtk.c.o

src/xlib/gtk.i: src/xlib/gtk.c.i

.PHONY : src/xlib/gtk.i

# target to preprocess a source file
src/xlib/gtk.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/gtk.c.i
.PHONY : src/xlib/gtk.c.i

src/xlib/gtk.s: src/xlib/gtk.c.s

.PHONY : src/xlib/gtk.s

# target to generate assembly for a file
src/xlib/gtk.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/gtk.c.s
.PHONY : src/xlib/gtk.c.s

src/xlib/main.o: src/xlib/main.c.o

.PHONY : src/xlib/main.o

# target to build an object file
src/xlib/main.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/main.c.o
.PHONY : src/xlib/main.c.o

src/xlib/main.i: src/xlib/main.c.i

.PHONY : src/xlib/main.i

# target to preprocess a source file
src/xlib/main.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/main.c.i
.PHONY : src/xlib/main.c.i

src/xlib/main.s: src/xlib/main.c.s

.PHONY : src/xlib/main.s

# target to generate assembly for a file
src/xlib/main.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/main.c.s
.PHONY : src/xlib/main.c.s

src/xlib/mmenu.o: src/xlib/mmenu.c.o

.PHONY : src/xlib/mmenu.o

# target to build an object file
src/xlib/mmenu.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/mmenu.c.o
.PHONY : src/xlib/mmenu.c.o

src/xlib/mmenu.i: src/xlib/mmenu.c.i

.PHONY : src/xlib/mmenu.i

# target to preprocess a source file
src/xlib/mmenu.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/mmenu.c.i
.PHONY : src/xlib/mmenu.c.i

src/xlib/mmenu.s: src/xlib/mmenu.c.s

.PHONY : src/xlib/mmenu.s

# target to generate assembly for a file
src/xlib/mmenu.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/mmenu.c.s
.PHONY : src/xlib/mmenu.c.s

src/xlib/v4l.o: src/xlib/v4l.c.o

.PHONY : src/xlib/v4l.o

# target to build an object file
src/xlib/v4l.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/v4l.c.o
.PHONY : src/xlib/v4l.c.o

src/xlib/v4l.i: src/xlib/v4l.c.i

.PHONY : src/xlib/v4l.i

# target to preprocess a source file
src/xlib/v4l.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/v4l.c.i
.PHONY : src/xlib/v4l.c.i

src/xlib/v4l.s: src/xlib/v4l.c.s

.PHONY : src/xlib/v4l.s

# target to generate assembly for a file
src/xlib/v4l.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/v4l.c.s
.PHONY : src/xlib/v4l.c.s

src/xlib/video.o: src/xlib/video.c.o

.PHONY : src/xlib/video.o

# target to build an object file
src/xlib/video.c.o:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/video.c.o
.PHONY : src/xlib/video.c.o

src/xlib/video.i: src/xlib/video.c.i

.PHONY : src/xlib/video.i

# target to preprocess a source file
src/xlib/video.c.i:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/video.c.i
.PHONY : src/xlib/video.c.i

src/xlib/video.s: src/xlib/video.c.s

.PHONY : src/xlib/video.s

# target to generate assembly for a file
src/xlib/video.c.s:
	$(MAKE) -f CMakeFiles/utox.dir/build.make CMakeFiles/utox.dir/src/xlib/video.c.s
.PHONY : src/xlib/video.c.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... edit_cache"
	@echo "... icon"
	@echo "... rebuild_cache"
	@echo "... utox"
	@echo "... src/av/audio.o"
	@echo "... src/av/audio.i"
	@echo "... src/av/audio.s"
	@echo "... src/av/utox_av.o"
	@echo "... src/av/utox_av.i"
	@echo "... src/av/utox_av.s"
	@echo "... src/av/video.o"
	@echo "... src/av/video.i"
	@echo "... src/av/video.s"
	@echo "... src/avatar.o"
	@echo "... src/avatar.i"
	@echo "... src/avatar.s"
	@echo "... src/commands.o"
	@echo "... src/commands.i"
	@echo "... src/commands.s"
	@echo "... src/devices.o"
	@echo "... src/devices.i"
	@echo "... src/devices.s"
	@echo "... src/dns.o"
	@echo "... src/dns.i"
	@echo "... src/dns.s"
	@echo "... src/file_transfers.o"
	@echo "... src/file_transfers.i"
	@echo "... src/file_transfers.s"
	@echo "... src/flist.o"
	@echo "... src/flist.i"
	@echo "... src/flist.s"
	@echo "... src/friend.o"
	@echo "... src/friend.i"
	@echo "... src/friend.s"
	@echo "... src/groups.o"
	@echo "... src/groups.i"
	@echo "... src/groups.s"
	@echo "... src/inline_video.o"
	@echo "... src/inline_video.i"
	@echo "... src/inline_video.s"
	@echo "... src/main.o"
	@echo "... src/main.i"
	@echo "... src/main.s"
	@echo "... src/messages.o"
	@echo "... src/messages.i"
	@echo "... src/messages.s"
	@echo "... src/theme.o"
	@echo "... src/theme.i"
	@echo "... src/theme.s"
	@echo "... src/tox.o"
	@echo "... src/tox.i"
	@echo "... src/tox.s"
	@echo "... src/tox_callbacks.o"
	@echo "... src/tox_callbacks.i"
	@echo "... src/tox_callbacks.s"
	@echo "... src/ui.o"
	@echo "... src/ui.i"
	@echo "... src/ui.s"
	@echo "... src/ui/button.o"
	@echo "... src/ui/button.i"
	@echo "... src/ui/button.s"
	@echo "... src/ui/buttons.o"
	@echo "... src/ui/buttons.i"
	@echo "... src/ui/buttons.s"
	@echo "... src/ui/contextmenu.o"
	@echo "... src/ui/contextmenu.i"
	@echo "... src/ui/contextmenu.s"
	@echo "... src/ui/draw_helpers.o"
	@echo "... src/ui/draw_helpers.i"
	@echo "... src/ui/draw_helpers.s"
	@echo "... src/ui/dropdown.o"
	@echo "... src/ui/dropdown.i"
	@echo "... src/ui/dropdown.s"
	@echo "... src/ui/dropdowns.o"
	@echo "... src/ui/dropdowns.i"
	@echo "... src/ui/dropdowns.s"
	@echo "... src/ui/edit.o"
	@echo "... src/ui/edit.i"
	@echo "... src/ui/edit.s"
	@echo "... src/ui/edits.o"
	@echo "... src/ui/edits.i"
	@echo "... src/ui/edits.s"
	@echo "... src/ui/scrollable.o"
	@echo "... src/ui/scrollable.i"
	@echo "... src/ui/scrollable.s"
	@echo "... src/ui/svg.o"
	@echo "... src/ui/svg.i"
	@echo "... src/ui/svg.s"
	@echo "... src/ui/switch.o"
	@echo "... src/ui/switch.i"
	@echo "... src/ui/switch.s"
	@echo "... src/ui/switches.o"
	@echo "... src/ui/switches.i"
	@echo "... src/ui/switches.s"
	@echo "... src/ui/text.o"
	@echo "... src/ui/text.i"
	@echo "... src/ui/text.s"
	@echo "... src/ui/tooltip.o"
	@echo "... src/ui/tooltip.i"
	@echo "... src/ui/tooltip.s"
	@echo "... src/ui_i18n.o"
	@echo "... src/ui_i18n.i"
	@echo "... src/ui_i18n.s"
	@echo "... src/util.o"
	@echo "... src/util.i"
	@echo "... src/util.s"
	@echo "... src/xlib/audio.o"
	@echo "... src/xlib/audio.i"
	@echo "... src/xlib/audio.s"
	@echo "... src/xlib/dbus.o"
	@echo "... src/xlib/dbus.i"
	@echo "... src/xlib/dbus.s"
	@echo "... src/xlib/drawing.o"
	@echo "... src/xlib/drawing.i"
	@echo "... src/xlib/drawing.s"
	@echo "... src/xlib/event.o"
	@echo "... src/xlib/event.i"
	@echo "... src/xlib/event.s"
	@echo "... src/xlib/freetype.o"
	@echo "... src/xlib/freetype.i"
	@echo "... src/xlib/freetype.s"
	@echo "... src/xlib/gtk.o"
	@echo "... src/xlib/gtk.i"
	@echo "... src/xlib/gtk.s"
	@echo "... src/xlib/main.o"
	@echo "... src/xlib/main.i"
	@echo "... src/xlib/main.s"
	@echo "... src/xlib/mmenu.o"
	@echo "... src/xlib/mmenu.i"
	@echo "... src/xlib/mmenu.s"
	@echo "... src/xlib/v4l.o"
	@echo "... src/xlib/v4l.i"
	@echo "... src/xlib/v4l.s"
	@echo "... src/xlib/video.o"
	@echo "... src/xlib/video.i"
	@echo "... src/xlib/video.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

