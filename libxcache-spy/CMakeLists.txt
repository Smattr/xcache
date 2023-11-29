project(libxcache-spy LANGUAGES C)

add_library(libxcache-spy SHARED
  src/init.c
)

if(APPLE)
  target_compile_options(libxcache-spy PRIVATE -fno-common)
endif()

set_target_properties(libxcache-spy PROPERTIES PREFIX "")

install(TARGETS libxcache-spy EXPORT LibxcachespyConfig
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

export(TARGETS libxcache-spy FILE LibxcachespyConfig.cmake)
