add_library(libxcache
  include/xcache/proc.h
  include/xcache/xcache.h

  src/macros.h
  src/proc.h

  src/proc_exec.c
  src/proc_free.c
  src/proc_new.c
)

target_include_directories(libxcache
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    src
)

set_target_properties(libxcache PROPERTIES PREFIX "")

install(TARGETS libxcache EXPORT LibxcacheConfig
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

export(TARGETS libxcache FILE LibxcacheConfig.cmake)
