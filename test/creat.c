/// @file
/// @brief Test recording of `creat`

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc __attribute__((unused)), char **argv) {

  const int fd = creat("foo", 0644);
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
