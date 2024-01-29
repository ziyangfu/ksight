# This is a CMake toolchain include file
# This file contains the gcc debug build configuration.
set(CXX_DEBUG_FLAGS "-Wfatal-errors")
set(C_DEBUG_FLAGS "-Wfatal-errors")

set_optimization_level("0")
