# Basic usage:
#  `cmake .` to generate a Makefile. `cmake . -DARCH=...` if you want to build
#  for a different architecture than your host machine.
#

cmake_minimum_required (VERSION 2.6)

project (xcache)

set (CMAKE_C_FLAGS "-std=gnu99 -W -Wall -Wextra")

set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)

# Dump CMake settings so we can use them from C.
configure_file (
    "${PROJECT_SOURCE_DIR}/config.in.h"
    "${PROJECT_BINARY_DIR}/config.h")
include_directories ("${PROJECT_BINARY_DIR}")

# Handle building for different architectures.
execute_process (
    COMMAND uname -m
    OUTPUT_VARIABLE HOST_ARCH
    OUTPUT_STRIP_TRAILING_WHITESPACE)
option(ARCH "architecture" ${HOST_ARCH})
if (NOT IS_DIRECTORY "${PROJECT_SOURCE_DIR}/arch/${ARCH}")
    message (SEND_ERROR "Your chosen architecture, ${ARCH}, is not implemented")
elseif (NOT "${ARCH}" STREQUAL "${HOST_ARCH}")
    message (STATUS "Warning: setting an architecture, ${ARCH}, different from your host, ${HOST_ARCH}")
endif ()
include_directories ("${PROJECT_SOURCE_DIR}/arch/${ARCH}")

# The final executable we want.
add_executable (xcache debug.c depset.c dict.c main.c trace.c)
