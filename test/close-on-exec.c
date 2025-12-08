/// @file
/// @brief Test of close-on-exec semantics
///
/// This program opens a file with close-on-exec set and then execs. If xcache
/// is modelling this correctly, it should understand the file descriptor is
/// closed after this sequence.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int __attribute__((unused)) argc, char **argv) {

  // open a file with close-on-exec set
  const int fd = open("/dev/null", O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    fprintf(stderr, "%s: open failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  // exec our child, passing it the information of this descriptor
  char arg0[] = "xcache-test-close-fd2";
  char arg1[128] = {0};
  snprintf(arg1, sizeof(arg1), "%d", fd);
  char *const args[] = {arg0, arg1, NULL};
  execvp(arg0, args);

  fprintf(stderr, "%s: execvp failed: %s\n", argv[0], strerror(errno));
  return EXIT_FAILURE;
}
