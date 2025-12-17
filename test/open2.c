/// @file
/// @brief Open a file using the `open` syscall
///
/// Attempting to open a file with the `open` libc function typically bottoms
/// out on `openat`. So to test `open` we need to call it explicitly.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(int argc __attribute__((unused)), char **argv) {

  const int fd =
      syscall(SYS_open, "foo", (long)(O_WRONLY | O_CREAT | O_TRUNC), 0644L);
  if (fd < 0) {
    fprintf(stderr, "%s: open failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  if (write(fd, "bar", strlen("bar")) < 0) {
    fprintf(stderr, "%s: write failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  (void)close(fd);

  return EXIT_SUCCESS;
}
