cmake_minimum_required (VERSION 2.6)
project (xcache)

###############################################################################
# General Configuration                                                       #
###############################################################################

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu1x -W -Wall -Wextra")
add_definitions (-DXCACHE_PIPE="XCACHE_PIPE"
                 -DVERSION_MAJOR=0
                 -DVERSION_MINOR=1)

###############################################################################
# Debug or Release                                                            #
###############################################################################

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release)
endif (NOT CMAKE_BUILD_TYPE)
if (CMAKE_BUILD_TYPE MATCHES Debug)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb")
else (CMAKE_BUILD_TYPE MATCHES Debug)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -DNDEBUG")
endif (CMAKE_BUILD_TYPE MATCHES Debug)
find_package (PkgConfig)

###############################################################################
# Dependencies                                                                #
###############################################################################

pkg_check_modules (GLIB REQUIRED glib-2.0)
include_directories (${GLIB_INCLUDE_DIRS})

find_package (Threads)

pkg_check_modules (SQLITE3 REQUIRED sqlite3)
include_directories (${SQLITE3_INCLUDE_DIRS})

pkg_check_modules (OPENSSL REQUIRED openssl)
include_directories (${OPENSSL_INCLUDE_DIRS})

find_library (CUNIT NAMES cunit libcunit cunitlib)

###############################################################################
# Target architecture                                                         #
###############################################################################

set (ARCH "${ARCH}" CACHE STRING "architecture to target")
if ("${ARCH}" STREQUAL "")
  set (ARCH "${CMAKE_SYSTEM_PROCESSOR}")
endif ("${ARCH}" STREQUAL "")
if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/arch/${ARCH})
  message (FATAL_ERROR "your chosen architecture, ${ARCH}, is not implemented")
endif (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/arch/${ARCH})
message (STATUS "configuring build for ${ARCH} architecture")
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/arch/${ARCH})

###############################################################################
# Targets                                                                     #
###############################################################################

add_executable (oversee oversee.c translate-syscall.c)
target_link_libraries (oversee xcache ${GLIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_library (hook comm-protocol.c getenv.c message-protocol.c)

set (LIBXCACHE_SOURCES cache.c collection/list.c comm-protocol.c db.c depset.c
                       collection/dict.c fingerprint.c hook.c log.c
                       ptrace-wrapper.c trace.c util/abspath.c util/aprintf.c
                       util/cp.c util/du.c util/filehash.c util/fileiter.c
                       util/get.c util/mkdirp.c util/ralloc.c util/reduce.c
                       message-protocol.c util/readlink.c util/resolve.c)
set (LIBXCACHE_LIBS ${GLIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
                    ${SQLITE3_LIBRARIES} ${OPENSSL_LIBRARIES})
add_library (xcache ${LIBXCACHE_SOURCES})
target_link_libraries (xcache ${LIBXCACHE_LIBS})

add_executable (xcache-bin main.c translate-syscall.c)
set_target_properties (xcache-bin
    PROPERTIES OUTPUT_NAME xcache)
target_link_libraries (xcache-bin xcache)

add_executable (xcache-tests test.c ${LIBXCACHE_SOURCES})
target_link_libraries (xcache-tests ${LIBXCACHE_LIBS} ${CUNIT})
set_target_properties (xcache-tests PROPERTIES COMPILE_FLAGS -DXCACHE_TEST=1)
