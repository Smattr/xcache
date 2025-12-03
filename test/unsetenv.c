/// @file
/// @brief Test of xcache’s ability to track `unsetenv` calls
///
/// See test.py::test_unsetenv

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc __attribute__((unused)), char **argv) {

  // delete an environment variable
  if (unsetenv("FOO") < 0) {
    fprintf(stderr, "%s: unsetenv failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  // if xcache did not correctly model our `unsetenv`, it will incorrectly
  // conclude this `getenv` represents a dependency, interfering with
  // replayability
  const char *const foo = getenv("FOO");
  printf("$FOO = %s\n", foo == NULL ? "<NULL>" : foo);

  return EXIT_SUCCESS;
}
