/// @file
/// @brief Test of xcache’s ability to track `putenv` calls
///
/// See test.py::test_putenv

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char name_value[] = "FOO=bar";

int main(int argc __attribute__((unused)), char **argv) {

  // overwrite the value of an environment variable
  if (putenv(name_value) != 0) {
    fprintf(stderr, "%s: putenv failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  // if xcache did not correctly model our `putenv`, it will incorrectly
  // conclude this `getenv` represents a dependency, interfering with
  // replayability
  const char *const foo = getenv("FOO");
  printf("$FOO = %s\n", foo);

  return EXIT_SUCCESS;
}
