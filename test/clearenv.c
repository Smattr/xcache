/// @file
/// @brief Test of xcache’s ability to track `clearenv` calls
///
/// See test.py::test_clearenv

#include <stdio.h>
#include <stdlib.h>

int main(int argc __attribute__((unused)), char **argv) {

  // clear the environment
  if (clearenv() != 0) {
    fprintf(stderr, "%s: clearenv failed\n", argv[0]);
    return EXIT_FAILURE;
  }

  // if xcache did not correctly model our `clearenv`, it will incorrectly
  // conclude this `getenv` represents a dependency, interfering with
  // replayability
  const char *const foo = getenv("FOO");
  printf("$FOO = %s\n", foo);

  return EXIT_SUCCESS;
}
