add_executable(xcache
  src/main.c
)

target_include_directories(xcache
  PRIVATE
    src
)

target_link_libraries(xcache libxcache)

add_custom_target(man-xcache
  ALL
  DEPENDS xcache.1.gz)
add_custom_command(
  OUTPUT xcache.1.gz
  COMMAND gzip -9 --no-name --to-stdout doc/xcache.1 >"${CMAKE_CURRENT_BINARY_DIR}/xcache.1.gz"
  MAIN_DEPENDENCY doc/xcache.1
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS xcache
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/xcache.1.gz
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
