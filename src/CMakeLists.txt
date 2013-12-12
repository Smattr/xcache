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
    COMMAND "${CMAKE_SOURCE_DIR}/sql/to_c.sh" "${CMAKE_BINARY_DIR}/queries.c" "${CMAKE_SOURCE_DIR}/sql/addinput.sql" "${CMAKE_SOURCE_DIR}/sql/addoperation.sql" "${CMAKE_SOURCE_DIR}/sql/addoutput.sql" "${CMAKE_SOURCE_DIR}/sql/create.sql" "${CMAKE_SOURCE_DIR}/sql/getid.sql" "${CMAKE_SOURCE_DIR}/sql/getinputs.sql" "${CMAKE_SOURCE_DIR}/sql/truncate.sql"
    DEPENDS "${CMAKE_SOURCE_DIR}/sql/create.sql"
    DEPENDS "${CMAKE_SOURCE_DIR}/sql/truncate.sql"
    VERBATIM)

# Dump CMake settings so we can use them from C.
configure_file (
    "${PROJECT_SOURCE_DIR}/config.in.h"
    "${PROJECT_BINARY_DIR}/config.h")
include_directories ("${PROJECT_BINARY_DIR}")

# The final executable we want.
add_executable (xcache cache.c depset.c dict.c file.c log.c main.c queries.c trace.c)

# -lpthread
find_package (Threads)
target_link_libraries (xcache ${CMAKE_THREAD_LIBS_INIT})

# -ldl
target_link_libraries (xcache ${CMAKE_DL_LIBS})

# -lsqlite3
find_library (SQLITE sqlite3)
target_link_libraries (xcache ${SQLITE})

# -lopenssl
find_library (OPENSSL ssl)
target_link_libraries (xcache ${OPENSSL})

# -lcrypto
find_library (CRYPTO crypto)
target_link_libraries (xcache ${CRYPTO})
