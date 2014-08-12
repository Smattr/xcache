# Basic usage:
#  `cmake .` to generate a Makefile. `cmake . -DARCH=...` if you want to build
#  for a different architecture than your host machine.
#

cmake_minimum_required (VERSION 2.6)

project (xcache)

set (CMAKE_C_FLAGS "-std=gnu99 -W -Wall -Wextra")

set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)
set (PROJ_NAME "\"xcache\"")

set (LIBSOURCES cache.c collection/list.c db.c depset.c collection/dict.c fingerprint.c log.c ptrace-wrapper.c collection/set.c tee.c trace.c util/abspath.c util/cp.c util/du.c util/filehash.c util/fileiter.c util/get.c util/mkdirp.c util/reduce.c)
set (EXESOURCES main.c translate-syscall.c)
set (GETENVSOURCES hook-getenv.c)
set (TESTSOURCES test.c)

# Handle building for different architectures.
execute_process (
    COMMAND uname -m
    OUTPUT_VARIABLE HOST_ARCH
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if (NOT DEFINED ARCH)
    set (ARCH ${HOST_ARCH})
endif ()
message (STATUS "Configuring build for ${ARCH} architecture")
if (NOT IS_DIRECTORY "${PROJECT_SOURCE_DIR}/arch/${ARCH}")
    message (SEND_ERROR "Your chosen architecture is not implemented")
endif ()
include_directories ("${PROJECT_SOURCE_DIR}/arch/${ARCH}")

# Dump CMake settings so we can use them from C.
configure_file (
    "${PROJECT_SOURCE_DIR}/config.in.h"
    "${PROJECT_BINARY_DIR}/config.h")
include_directories ("${PROJECT_BINARY_DIR}")

# Go on a long, and often unfulfilling, journey to find GLib.
find_path (
  GLIB_CORE_INCLUDES glib.h
  PATHS ${CMAKE_EXTRA_INCLUDES} PATH_SUFFIXES glib-2.0/ glib-2.0/include NO_DEFAULT_PATH)
if (NOT GLIB_CORE_INCLUDES)
    find_path (
      GLIB_CORE_INCLUDES glib.h
      PATHS /usr/local/include /usr/include /include /sw/include /usr/lib /usr/lib64 /usr/lib/x86_64-linux-gnu/ ${CMAKE_EXTRA_INCLUDES} PATH_SUFFIXES glib-2.0/ glib-2.0/include)
endif (NOT GLIB_CORE_INCLUDES)
if (NOT GLIB_CORE_INCLUDES)
    message ("warning: failed to find glib.h")
endif (NOT GLIB_CORE_INCLUDES)
include_directories (${GLIB_CORE_INCLUDES})

find_path (
  GLIB_CONFIG_INCLUDES glibconfig.h
  PATHS ${CMAKE_EXTRA_INCLUDES} ${CMAKE_EXTRA_LIBRARIES} PATH_SUFFIXES glib-2.0/ glib-2.0/include NO_DEFAULT_PATH)
if (NOT GLIB_CONFIG_INCLUDES)
    find_path (
      GLIB_CONFIG_INCLUDES glibconfig.h
      PATHS /usr/local/include /usr/include /include /sw/include /usr/lib /usr/lib64 /usr/lib/x86_64-linux-gnu/ /usr/lib/i386-linux-gnu ${CMAKE_EXTRA_INCLUDES} ${CMAKE_EXTRA_LIBRARIES} PATH_SUFFIXES glib-2.0 glib-2.0/include)
endif (NOT GLIB_CONFIG_INCLUDES)
if (NOT GLIB_CONFIG_INCLUDES)
    message ("warning: failed to find glibconfig.h")
endif (NOT GLIB_CONFIG_INCLUDES)
include_directories (${GLIB_CONFIG_INCLUDES})

# Build xcache as a library in case anyone wants to reuse it.
add_library (libxcache ${LIBSOURCES})
set_target_properties (libxcache PROPERTIES PREFIX "")
link_directories (${CMAKE_BINARY_DIR})

# Build xcache as a shared library just in case anyone needs that instead.
add_library (libxcache.so SHARED ${LIBSOURCES})
set_target_properties (libxcache.so PROPERTIES PREFIX "")
set_target_properties (libxcache.so PROPERTIES SUFFIX "")

# The final executable we want.
add_executable (xcache ${EXESOURCES})
target_link_libraries (xcache libxcache)

add_executable (xcache-tests ${TESTSOURCES} ${LIBSOURCES})
set_target_properties (xcache-tests PROPERTIES COMPILE_DEFINITIONS "XCACHE_TEST=1")

add_library (libhookgetenv.so SHARED ${GETENVSOURCES})
set_target_properties (libhookgetenv.so PROPERTIES PREFIX "")
set_target_properties (libhookgetenv.so PROPERTIES SUFFIX "")

# File descriptor (in target's descriptor space) on which Xcache and the target
# can communicate out-of-band extra information. Currently only used for
# environment variable access.
set_target_properties (xcache PROPERTIES COMPILE_DEFINITIONS "XCACHE_FILENO=3")
set_target_properties (libhookgetenv.so PROPERTIES COMPILE_DEFINITIONS "XCACHE_FILENO=3")

find_library (
  GLIB NAMES glib-2.0
  PATHS ${CMAKE_EXTRA_LIBRARIES} PATH_SUFFIXES glib-2.0/ NO_DEFAULT_PATH)
if (NOT GLIB)
    find_library (
      GLIB NAMES glib-2.0
      PATHS /usr/local/lib /usr/lib /lib /sw/lib ${CMAKE_EXTRA_LIBRARIES} PATH_SUFFIXES glib-2.0/)
endif (NOT GLIB)
target_link_libraries (xcache ${GLIB})
target_link_libraries (xcache-tests ${GLIB})
find_library (PTHREADS pthread)
target_link_libraries (xcache ${PTHREADS})
target_link_libraries (xcache-tests ${PTHREADS})
find_library (DL dl)
target_link_libraries (xcache ${DL})
target_link_libraries (xcache-tests ${DL})
find_library (SQLITE sqlite3)
target_link_libraries (xcache ${SQLITE})
target_link_libraries (xcache-tests ${SQLITE})
find_library (OPENSSL ssl)
target_link_libraries (xcache ${OPENSSL})
target_link_libraries (xcache-tests ${OPENSSL})
find_library (CRYPTO crypto)
target_link_libraries (xcache ${CRYPTO})
target_link_libraries (xcache-tests ${CRYPTO})
