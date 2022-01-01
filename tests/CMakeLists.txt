add_executable(my-echo my-echo.c)
add_executable(my-cat my-cat.c)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_program(STRACE strace)

add_custom_target(check
  COMMAND env PATH=${CMAKE_CURRENT_BINARY_DIR}:${CMAKE_CURRENT_BINARY_DIR}/../xcache:$ENV{PATH} ${Python3_EXECUTABLE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/tests.py --verbose)
add_dependencies(check my-cat my-echo xcache)
