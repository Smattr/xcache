/// @file
/// @brief Test of `umask` effect
///
/// This is a variant of umask-open.c that sets a umask that _will_ result in
/// masking out some mode bits. In order to replicate this accurately, xcache
/// needs some understanding of umasks.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char **argv) {

  (void)argc;

  // set a `umask` that will mask off x mode bits
  (void)umask(0111);

  // now create a file with a mode that should be (partially) masked out
  const int fd = open("foo", O_WRONLY | O_CREAT | O_TRUNC, 0777);
  if (fd < 0) {
    fprintf(stderr, "%s: open failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  (void)close(fd);

  return EXIT_SUCCESS;
}
