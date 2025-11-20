/// @file
/// @brief Close a given file descriptor that is assumed to already be closed
///
/// See close-on-exec.c for the purpose of this program.

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "expected usage: %s <fd>\n", argv[0]);
    return EXIT_FAILURE;
  }

  int fd;
  if (sscanf(argv[1], "%d", &fd) != 1) {
    fprintf(stderr, "%s: sscanf failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  const int closed = close(fd);
  if (closed < 0 && errno != EBADF) {
    fprintf(stderr, "%s: close failed with unexpected result: %s\n", argv[0],
            strerror(errno));
    return EXIT_FAILURE;
  } else if (closed == 0) {
    fprintf(stderr, "%s: close unexpectedly succeeded\n", argv[0]);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
