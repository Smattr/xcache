/// @file
/// @brief Test recording of `creat`
///
/// `creat` can be implemented in terms of `open` or `openat`. So it is possible
/// that creat.c does not actually test that we can trace `creat`. This program
/// does the same but explicitly invokes `creat` to confirm we can also trace
/// that.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(int argc __attribute__((unused)), char **argv) {

  const int fd = syscall(SYS_creat, "foo", 0644L);
  if (fd < 0) {
    fprintf(stderr, "%s: creat failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  if (write(fd, "bar", strlen("bar")) < 0) {
    fprintf(stderr, "%s: write failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  (void)close(fd);

  return EXIT_SUCCESS;
}
