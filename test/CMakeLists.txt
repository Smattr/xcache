# Even if we have been asked to enable sanitizers, disable them for test
# binaries. The sanitizers, in particular ASan, cause tracees to make extra
# syscalls that are frequently ones we cannot replay.
if(HAS_NO_SANITIZE_ALL)
  add_compile_options(-fno-sanitize=all)
  add_link_options(-fno-sanitize=all)
endif()

add_executable(xcache-test-clone-exec-with-fd clone-exec-with-fd.c)
add_executable(xcache-test-close-fd close-fd.c)
add_executable(xcache-test-close-fd2 close-fd2.c)
add_executable(xcache-test-close-on-exec close-on-exec.c)
add_executable(xcache-test-forker forker.c)
add_executable(xcache-test-forker-fork forker-fork.c)
add_executable(xcache-test-forker-vfork forker-vfork.c)
add_executable(xcache-test-ld-preload-in-child ld-preload-in-child.c)
add_executable(xcache-test-echo echo.c)
add_executable(xcache-test-execvp execvp.c)
add_executable(xcache-test-sysconf sysconf.c)
add_executable(xcache-test-nop nop.c)
add_executable(xcache-test-print-stderr print-stderr.c)
add_executable(xcache-test-print-stdout print-stdout.c)
add_executable(xcache-test-umask-open umask-open.c)
add_executable(xcache-test-umask-open2 umask-open2.c)
add_executable(xcache-test-uncacheable uncacheable.c)
add_executable(xcache-test-write-file write-file.c)
add_executable(xcache-test-bug-set-size bug-set-size.c)
add_executable(xcache-test-clearenv clearenv.c)
add_executable(xcache-test-creat creat.c)
add_executable(xcache-test-creat2 creat2.c)
add_executable(xcache-test-fd-without-path fd-without-path.c)
add_executable(xcache-test-getenv getenv.c)
add_executable(xcache-test-non-path-fds non-path-fds.c)
add_executable(xcache-test-open open.c)
add_executable(xcache-test-open2 open2.c)
add_executable(xcache-test-putenv putenv.c)
add_executable(xcache-test-setenv setenv.c)
add_executable(xcache-test-temp-usage temp-usage.c)
add_executable(xcache-test-unlink unlink.c)
add_executable(xcache-test-unsetenv unsetenv.c)

add_library(xcache-test-ld-preload-cos SHARED preload.c)
find_library(M m REQUIRED)
add_executable(xcache-test-ld-preload preload-main.c)
target_link_libraries(xcache-test-ld-preload PRIVATE ${M})

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
  xcache-test-clone-exec-with-fd
  xcache-test-close-fd
  xcache-test-close-fd2
  xcache-test-close-on-exec
  xcache-test-forker
  xcache-test-forker-fork
  xcache-test-forker-vfork
  xcache-test-ld-preload-in-child
  xcache-test-echo
  xcache-test-execvp
  xcache-test-sysconf
  xcache-test-nop
  xcache-test-print-stderr
  xcache-test-print-stdout
  xcache-test-umask-open
  xcache-test-umask-open2
  xcache-test-uncacheable
  xcache-test-write-file
  libxcache-spy
  xcache
  xcache-test-bug-set-size
  xcache-test-clearenv
  xcache-test-creat
  xcache-test-creat2
  xcache-test-fd-without-path
  xcache-test-getenv
  xcache-test-ld-preload
  xcache-test-ld-preload-cos
  xcache-test-non-path-fds
  xcache-test-open
  xcache-test-open2
  xcache-test-putenv
  xcache-test-setenv
  xcache-test-temp-usage
  xcache-test-unlink
  xcache-test-unsetenv
)
