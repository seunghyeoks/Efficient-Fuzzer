# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /workspace/Efficient-Fuzzer/src/fprime

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native-ut

# Utility rule file for Svc_CmdDispatcher_check.

# Include any custom commands dependencies for this target.
include F-Prime/Svc/CmdDispatcher/CMakeFiles/Svc_CmdDispatcher_check.dir/compiler_depend.make

# Include the progress variables for this target.
include F-Prime/Svc/CmdDispatcher/CMakeFiles/Svc_CmdDispatcher_check.dir/progress.make

F-Prime/Svc/CmdDispatcher/CMakeFiles/Svc_CmdDispatcher_check:
	cd /workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native-ut/F-Prime/Svc/CmdDispatcher && /usr/bin/ctest --verbose

Svc_CmdDispatcher_check: F-Prime/Svc/CmdDispatcher/CMakeFiles/Svc_CmdDispatcher_check
Svc_CmdDispatcher_check: F-Prime/Svc/CmdDispatcher/CMakeFiles/Svc_CmdDispatcher_check.dir/build.make
.PHONY : Svc_CmdDispatcher_check

# Rule to build all files generated by this target.
F-Prime/Svc/CmdDispatcher/CMakeFiles/Svc_CmdDispatcher_check.dir/build: Svc_CmdDispatcher_check
.PHONY : F-Prime/Svc/CmdDispatcher/CMakeFiles/Svc_CmdDispatcher_check.dir/build

F-Prime/Svc/CmdDispatcher/CMakeFiles/Svc_CmdDispatcher_check.dir/clean:
	cd /workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native-ut/F-Prime/Svc/CmdDispatcher && $(CMAKE_COMMAND) -P CMakeFiles/Svc_CmdDispatcher_check.dir/cmake_clean.cmake
.PHONY : F-Prime/Svc/CmdDispatcher/CMakeFiles/Svc_CmdDispatcher_check.dir/clean

F-Prime/Svc/CmdDispatcher/CMakeFiles/Svc_CmdDispatcher_check.dir/depend:
	cd /workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native-ut && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /workspace/Efficient-Fuzzer/src/fprime /workspace/Efficient-Fuzzer/src/fprime/Svc/CmdDispatcher /workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native-ut /workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native-ut/F-Prime/Svc/CmdDispatcher /workspace/Efficient-Fuzzer/src/fprime/build-fprime-automatic-native-ut/F-Prime/Svc/CmdDispatcher/CMakeFiles/Svc_CmdDispatcher_check.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : F-Prime/Svc/CmdDispatcher/CMakeFiles/Svc_CmdDispatcher_check.dir/depend

