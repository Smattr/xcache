/// @file
/// @brief Test of xcache’s ability to track `getenv` calls
///
/// See test.py::test_getenv.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc __attribute__((unused)), char **argv) {

  if (getenv("FOO") == NULL)
    return EXIT_SUCCESS;

  const int fd = open("foo", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    fprintf(stderr, "%s: open failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  const char CONTENT[] = "hello world";
  if (write(fd, CONTENT, strlen(CONTENT)) < 0) {
    fprintf(stderr, "%s: write failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  (void)close(fd);

  return EXIT_SUCCESS;
}
