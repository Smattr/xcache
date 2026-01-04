/// @file
/// @brief Create an unreadable file
///
/// It was observed that xcache returned counter-intuitive error messages when
/// one of the outputs it had seen was not readable at the time of saving the
/// trace. This program attempts to recreate such a situation to test whether we
/// can handle it gracefully.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc __attribute__((unused)), char **argv) {

  // create a file
  const int fd = open("foo", O_WRONLY | O_CREAT | O_EXCL | O_TRUNC, 0600);
  if (fd < 0) {
    fprintf(stderr, "%s: open failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  // write some content to it
  const char CONTENT[] = "hello world";
  if (write(fd, CONTENT, sizeof(CONTENT) - 1) < 0) {
    fprintf(stderr, "%s: write failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  (void)close(fd);

  // now chmod the file so we ourselves cannot read it
  if (chmod("foo", 0000) < 0) {
    fprintf(stderr, "%s: chmod failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
