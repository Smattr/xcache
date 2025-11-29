add_executable(clone-exec-with-fd clone-exec-with-fd.c)
add_executable(close-fd close-fd.c)
add_executable(close-fd2 close-fd2.c)
add_executable(close-on-exec close-on-exec.c)
add_executable(forker forker.c)
add_executable(forker-fork forker-fork.c)
add_executable(forker-vfork forker-vfork.c)
add_executable(ld-preload-in-child ld-preload-in-child.c)
add_executable(my-echo my-echo.c)
add_executable(my-execvp my-execvp.c)
add_executable(my-sysconf my-sysconf.c)
add_executable(nop nop.c)
add_executable(print-stderr print-stderr.c)
add_executable(print-stdout print-stdout.c)
add_executable(umask-open umask-open.c)
add_executable(umask-open2 umask-open2.c)
add_executable(uncacheable uncacheable.c)
add_executable(write-file write-file.c)

find_package(Python3 COMPONENTS Interpreter)
if(NOT Python3_FOUND)
  message(WARNING "python3 not found; disabling make check")
  return()
endif()

execute_process(
  COMMAND ${Python3_EXECUTABLE} -c "import pytest"
  RESULT_VARIABLE IMPORT_PYTEST_RET
  OUTPUT_QUIET
  ERROR_QUIET
)
if(NOT ${IMPORT_PYTEST_RET} EQUAL 0)
  message(WARNING "pytest not found; disabling make check")
  return()
endif()

add_custom_target(check
  COMMAND
    env PATH=${CMAKE_CURRENT_BINARY_DIR}:${CMAKE_BINARY_DIR}/xcache:$ENV{PATH}
    ${Python3_EXECUTABLE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/test.py
    --verbose
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
add_dependencies(check
  clone-exec-with-fd
  close-fd
  close-fd2
  close-on-exec
  forker
  forker-fork
  forker-vfork
  ld-preload-in-child
  my-echo
  my-execvp
  my-sysconf
  nop
  print-stderr
  print-stdout
  umask-open
  umask-open2
  uncacheable
  write-file
  libxcache-spy
  xcache
)
