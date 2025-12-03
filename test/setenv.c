/// @file
/// @brief Test of xcache’s ability to track `setenv` calls
///
/// See test.py::test_setenv

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc __attribute__((unused)), char **argv) {

  // overwrite the value of an environment variable
  if (setenv("FOO", "bar", 1) < 0) {
    fprintf(stderr, "%s: setenv failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  // if xcache did not correctly model our `setenv`, it will incorrectly
  // conclude this `getenv` represents a dependency, interfering with
  // replayability
  const char *const foo = getenv("FOO");
  printf("$FOO = %s\n", foo);

  return EXIT_SUCCESS;
}
