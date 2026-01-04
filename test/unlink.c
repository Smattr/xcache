/// @file
/// @brief Try removing a file
///
/// See test.py::test_unlink.

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc __attribute__((unused)), char **argv) {
  if (unlink("bar/foo") < 0) {
    fprintf(stderr, "%s: unlink failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
