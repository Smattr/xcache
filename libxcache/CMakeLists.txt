add_library(libxcache
  include/xcache/debug.h
  include/xcache/hash.h
  include/xcache/proc.h
  include/xcache/trace.h
  include/xcache/xcache.h

  src/channel.h
  src/copy_file.h
  src/debug.h
  src/make_hash.h
  src/macros.h
  src/proc.h
  src/trace.h

  src/channel_close.c
  src/channel_open.c
  src/channel_read.c
  src/channel_write.c
  src/copy_file.c
  src/debug.c
  src/hash_data.c
  src/hash_file.c
  src/make_hash.c
  src/proc_exec.c
  src/proc_free.c
  src/proc_new.c
  src/set_debug.c
  src/trace_record.c
  src/trace_replay.c
  src/trace_is_valid.c
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
