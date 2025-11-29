/// @file
/// @brief Run `execvp` with given parameters

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {

  if (argc < 2) {
    fprintf(stderr, "%s: missing arguments\n", argv[0]);
    return EXIT_FAILURE;
  }

  execvp(argv[1], &argv[1]);

  fprintf(stderr, "%s: execvp failed: %s\n", argv[0], strerror(errno));
  return EXIT_FAILURE;
}
