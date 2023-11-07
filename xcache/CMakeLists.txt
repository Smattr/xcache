project(xcache LANGUAGES C)

add_executable(xcache
  src/help.c
  src/main.c
  src/xasprintf.c
  src/xstrdup.c
  ${CMAKE_CURRENT_BINARY_DIR}/manpage.c
)

target_compile_definitions(xcache PRIVATE -D_GNU_SOURCE)

target_include_directories(xcache PRIVATE src)

target_link_libraries(xcache PRIVATE libxcache)

find_program(XXD xxd REQUIRED)
add_custom_command(OUTPUT manpage.c
  COMMAND ${XXD} -include xcache.1 ${CMAKE_CURRENT_BINARY_DIR}/manpage.c
  MAIN_DEPENDENCY src/xcache.1
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)

find_program(GZIP gzip REQUIRED)
add_custom_target(man-xcache
  ALL
  DEPENDS xcache.1.gz)
add_custom_command(OUTPUT xcache.1.gz
  COMMAND ${GZIP} -9 --no-name --to-stdout src/xcache.1
    >"${CMAKE_CURRENT_BINARY_DIR}/xcache.1.gz"
  MAIN_DEPENDENCY src/xcache.1
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS xcache
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/xcache.1.gz
  DESTINATION ${CMAKE_INSTALL_DIR}/man1)
