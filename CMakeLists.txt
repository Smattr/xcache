cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(xcache C)

include(GNUInstallDirs)

add_compile_options(-std=c11 -W -Wall -Wextra -Wformat=2 -Wwrite-strings
  -Wmissing-prototypes -Wshadow -Wundef)
add_definitions(-D_POSIX_C_SOURCE=200809L)

# enable --as-needed, present on GNU ld on Linux, to minimise dependencies
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed")
endif()

add_subdirectory(libxcache)
add_subdirectory(xcache)
