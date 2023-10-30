add_executable(my-echo my-echo.c)

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
)
add_dependencies(check
  my-echo
  xcache
)
