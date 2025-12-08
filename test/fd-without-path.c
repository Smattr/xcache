/// @file
/// @brief Attempt operations on a file descriptor without a path
///
/// Xcache sometimes looks up the path of a file descriptor, e.g. to record
/// `stat` calls. This can go wrong if the tracee `stat`s something that does
/// not have a conventional path and xcache does not account for this.

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main(__attribute__((unused)) int argc, char **argv) {

  // create a file descriptor that does not map to a path on disk
  const int fd = memfd_create("foo", 0);
  if (fd < 0) {
    fprintf(stderr, "%s: memfd_create failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  struct stat st;
  if (fstat(fd, &st) < 0) {
    fprintf(stderr, "%s: fstat failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  (void)close(fd);

  return EXIT_SUCCESS;
}
