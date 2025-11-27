project(libxcache-spy LANGUAGES C)

add_library(libxcache-spy SHARED
  src/call.c
  src/init.c
  ${CMAKE_CURRENT_BINARY_DIR}/version.c
)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_custom_command(
  OUTPUT version.c
  COMMAND ../common/make-version.py
    --preamble=\#include\ \\\"version.h\\\"
    --name=xc_spy_version
    --output=${CMAKE_CURRENT_BINARY_DIR}/version.c
  MAIN_DEPENDENCY ../common/make-version.py
  DEPENDS always_run
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# dummy output to make sure we always re-evaluate the version step above
add_custom_command(
  OUTPUT always_run
  COMMAND /usr/bin/env true)

target_compile_options(libxcache-spy PRIVATE -D_GNU_SOURCE)

if(APPLE)
  target_compile_options(libxcache-spy PRIVATE -fno-common)
endif()

target_include_directories(libxcache-spy
  PRIVATE
    src
)

set_target_properties(libxcache-spy PROPERTIES PREFIX "")

install(TARGETS libxcache-spy EXPORT LibxcachespyConfig
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

export(TARGETS libxcache-spy FILE LibxcachespyConfig.cmake)
