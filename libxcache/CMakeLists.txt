project(libxcache LANGUAGES C)

add_library(libxcache
  src/atfd_to_str.c
  src/cbor_read_str.c
  src/cbor_read_u64.c
  src/cbor_read_u64_raw.c
  src/cbor_write_str.c
  src/cbor_write_u64.c
  src/cbor_write_u64_raw.c
  src/cmd_load.c
  src/cmd_save.c
  src/cp.c
  src/db_trace_root.c
  src/fd_free.c
  src/fd_new.c
  src/hash_cmd.c
  src/hash_data.c
  src/hash_eq.c
  src/hash_file.c
  src/hash_load.c
  src/input_eq.c
  src/input_free.c
  src/input_is_valid.c
  src/input_load.c
  src/input_new_access.c
  src/input_new_read.c
  src/input_new_stat.c
  src/input_save.c
  src/openflags_to_str.c
  src/output_free.c
  src/output_load.c
  src/output_replay.c
  src/page_size.c
  src/path_absolute.c
  src/path_join.c
  src/peek_str.c
  src/proc_cont.c
  src/proc_end.c
  src/proc_exec.c
  src/proc_fd.c
  src/proc_fd_new.c
  src/proc_fds_free.c
  src/proc_free.c
  src/proc_input_new.c
  src/proc_new.c
  src/proc_save.c
  src/proc_signal.c
  src/proc_start.c
  src/proc_syscall.c
  src/statflags_to_str.c
  src/syscall_to_str.c
  src/sysenter.c
  src/sysenter_execve.c
  src/sysexit.c
  src/sysexit_access.c
  src/sysexit_chdir.c
  src/sysexit_close.c
  src/sysexit_open.c
  src/sysexit_stat.c
  src/trace_load.c
  src/trace_save.c
  src/xc_cmd_eq.c
  src/xc_cmd_exec.c
  src/xc_cmd_free.c
  src/xc_cmd_new.c
  src/xc_db_close.c
  src/xc_db_open.c
  src/xc_debug.c
  src/xc_record.c
  src/xc_record_modes.c
  src/xc_replay.c
  src/xc_set_debug.c
  src/xc_trace_find.c
  src/xc_trace_free.c
  src/xc_trace_is_valid.c
  src/xc_version_is_release.c
  ${CMAKE_CURRENT_BINARY_DIR}/version.c
)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_custom_command(
  OUTPUT version.c
  COMMAND src/make-version.py ${CMAKE_CURRENT_BINARY_DIR}/version.c
  MAIN_DEPENDENCY src/make-version.py
  DEPENDS always_run
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# dummy output to make sure we always re-evaluate the version step above
add_custom_command(
  OUTPUT always_run
  COMMAND /usr/bin/env true)

target_compile_options(libxcache PRIVATE -D_DEFAULT_SOURCE)
target_compile_options(libxcache PRIVATE -D_GNU_SOURCE)
target_compile_options(libxcache PRIVATE -D_POSIX_C_SOURCE=200809L)

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
