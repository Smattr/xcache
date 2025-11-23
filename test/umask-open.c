/// @file
/// @brief Test of `umask` effect
///
/// By default, a process’ umask prevents setting certain mode bits when
/// creating files. If a process changes its umask and then creates a file with
/// some of the mode bits that would previously have been masked out, xcache
/// needs to understand this to faithfully reproduce it.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char **argv) {

  (void)argc;

  // change our umask to something that allows all bits
  (void)umask(0);

  // now create a file with a mode that would normally be (partially) masked out
  const int fd = open("foo", O_WRONLY | O_CREAT | O_TRUNC, 0777);
  if (fd < 0) {
    fprintf(stderr, "%s: open failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  (void)close(fd);

  return EXIT_SUCCESS;
}
