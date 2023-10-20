project(libxcache LANGUAGES C)

add_library(libxcache
  src/cbor_read_str.c
  src/cbor_read_u64.c
  src/hash_data.c
  src/hash_eq.c
  src/hash_file.c
  src/hash_read.c
  src/input_free.c
  src/input_is_valid.c
  src/input_new.c
  src/input_read.c
  src/xc_db_close.c
  src/xc_db_open.c
  src/xc_debug.c
  src/xc_set_debug.c
  src/xc_trace_free.c
  src/xc_trace_is_valid.c
  src/xc_trace_record.c
)

if(APPLE)
  target_compile_options(libxcache PRIVATE -fno-common)
endif()

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
