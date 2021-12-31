cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(xcache C)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  message(FATAL_ERROR "xcache only runs on Linux")
endif()
string(REGEX MATCH "[0-9]+\.[0-9]+\.[0-9]+" KERNEL_VERSION ${CMAKE_SYSTEM_VERSION})
message(STATUS "Linux kernel ${KERNEL_VERSION} detected")
if(${KERNEL_VERSION} VERSION_LESS "3.5")
  message(FATAL_ERROR "Linux kernel too old to support PTRACE_O_TRACESECCOMP")
endif()
if (${KERNEL_VERSION} VERSION_LESS "4.8")
  message(FATAL_ERROR "Linux kernel too old for PTRACE_EVENT_SECCOMP behaviour")
endif()

include(GNUInstallDirs)

add_compile_options(-std=c11 -W -Wall -Wextra -Wformat=2 -Wwrite-strings
  -Wmissing-prototypes -Wshadow -Wundef)
add_definitions(-D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L)

# enable --as-needed to minimise dependencies
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed")

find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
  add_custom_target(format
    COMMAND find libxcache xcache tests \\\( -name "*.c" -or -name "*.h" \\\)
      -exec ${CLANG_FORMAT} -i --style=file {} \\\;
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

add_subdirectory(libxcache)
add_subdirectory(tests)
add_subdirectory(xcache)
