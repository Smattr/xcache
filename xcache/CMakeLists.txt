project(xcache LANGUAGES C)

add_executable(xcache
  src/main.c
  src/xasprintf.c
  src/xstrdup.c
)

target_compile_definitions(xcache PRIVATE -D_GNU_SOURCE)

target_include_directories(xcache PRIVATE src)

target_link_libraries(xcache PRIVATE libxcache)

install(TARGETS xcache
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
