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

set (LIBSOURCES abspath.c cache.c db.c depset.c dict.c util/file.c hash.c log.c ptrace-wrapper.c queries.c set.c tee.c trace.c util/cp.c util/filehash.c util/fileiter.c util/mkdirp.c util/reduce.c)
set (EXESOURCES main.c translate-syscall.c)

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

# Tell CMake how to build the bits of SQL as C strings.
ADD_CUSTOM_COMMAND (
    OUTPUT "${CMAKE_BINARY_DIR}/queries.c"
    COMMAND "${CMAKE_SOURCE_DIR}/sql/to_c.sh" "${CMAKE_BINARY_DIR}/queries.c" "${CMAKE_SOURCE_DIR}/sql/addinput.sql" "${CMAKE_SOURCE_DIR}/sql/addoperation.sql" "${CMAKE_SOURCE_DIR}/sql/addoutput.sql" "${CMAKE_SOURCE_DIR}/sql/create.sql" "${CMAKE_SOURCE_DIR}/sql/getid.sql" "${CMAKE_SOURCE_DIR}/sql/getinputs.sql" "${CMAKE_SOURCE_DIR}/sql/getoutputs.sql" "${CMAKE_SOURCE_DIR}/sql/truncate.sql"
    DEPENDS "${CMAKE_SOURCE_DIR}/sql/to_c.sh"
    DEPENDS "${CMAKE_SOURCE_DIR}/sql/addinput.sql"
    DEPENDS "${CMAKE_SOURCE_DIR}/sql/addoperation.sql"
    DEPENDS "${CMAKE_SOURCE_DIR}/sql/addoutput.sql"
    DEPENDS "${CMAKE_SOURCE_DIR}/sql/create.sql"
    DEPENDS "${CMAKE_SOURCE_DIR}/sql/getid.sql"
    DEPENDS "${CMAKE_SOURCE_DIR}/sql/getinputs.sql"
    DEPENDS "${CMAKE_SOURCE_DIR}/sql/getoutputs.sql"
    DEPENDS "${CMAKE_SOURCE_DIR}/sql/truncate.sql"
    VERBATIM)

# Dump CMake settings so we can use them from C.
configure_file (
    "${PROJECT_SOURCE_DIR}/config.in.h"
    "${PROJECT_BINARY_DIR}/config.h")
include_directories ("${PROJECT_BINARY_DIR}")

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

find_library (PTHREADS pthread)
target_link_libraries (xcache ${PTHREADS})
find_library (DL dl)
target_link_libraries (xcache ${DL})
find_library (SQLITE sqlite3)
target_link_libraries (xcache ${SQLITE})
find_library (OPENSSL ssl)
target_link_libraries (xcache ${OPENSSL})
find_library (CRYPTO crypto)
target_link_libraries (xcache ${CRYPTO})
