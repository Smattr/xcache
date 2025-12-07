/// @file
/// @brief
///
/// Open a file in a given mode. See test.py::test_open for discussion of this.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {

  // decide on `open` flags
  int flags = 0;
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "O_RDONLY") == 0) {
      flags |= O_RDONLY;
    } else if (strcmp(argv[i], "O_WRONLY") == 0) {
      flags |= O_WRONLY;
    } else if (strcmp(argv[i], "O_RDWR") == 0) {
      flags |= O_RDWR;
    } else if (strcmp(argv[i], "O_CREAT") == 0) {
      flags |= O_CREAT;
    } else if (strcmp(argv[i], "O_EXCL") == 0) {
      flags |= O_EXCL;
    } else if (strcmp(argv[i], "O_TRUNC") == 0) {
      flags |= O_TRUNC;
    } else {
      fprintf(stderr, "%s: unrecognised option: %s\n", argv[0], argv[i]);
      return EXIT_FAILURE;
    }
  }

  // pick an unusual mode to detect the effects of creation
  const mode_t mode = 0700;

  const int fd = open("foo", flags, mode);
  if (fd < 0)
    fprintf(stderr, "%s: open failed: %s\n", argv[0], strerror(errno));

  if (fd >= 0) {
    if (write(fd, "bar", strlen("bar")) < 0)
      fprintf(stderr, "%s: write failed: %s\n", argv[0], strerror(errno));
  }

  if (fd >= 0)
    (void)close(fd);

  return EXIT_SUCCESS;
}
