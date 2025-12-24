/// @file
/// @brief Use temporary files in the manner of a compiler
///
/// Some compilers like GCC work as an outer driver that coordinate various
/// sequential programs (compiler, assembler, linker, …). These programs pass
/// information to each other through temporary files. This program replicates a
/// simplified version of this scenario:
///   1. Write to a temporary file
///   2. Read back from this file
///   3. Delete the temporary file
/// Handling this requires a bit of intelligence for xcache to realise recording
/// or replaying any of these actions is irrelevant.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc __attribute((unused)), char **argv) {

  char *path = NULL;
  int fd = -1;
  int rc = 0;

  // where should we create temporary files?
  const char *tmp = getenv("TMPDIR");
  if (tmp == NULL)
    tmp = "/tmp";

  // create a temporary file
  if (asprintf(&path, "%s/tmp.XXXXXX", tmp) < 0) {
    rc = ENOMEM;
    fprintf(stderr, "%s: asprintf failed\n", argv[0]);
    goto done;
  }
  fd = mkstemp(path);
  if (fd < 0) {
    rc = errno;
    fprintf(stderr, "%s: mkstemp failed: %s\n", argv[0], strerror(rc));
    goto done;
  }

  // write some sample content to it
  const char CONTENT[] = "hello world";
  if (write(fd, CONTENT, strlen(CONTENT)) < 0) {
    rc = errno;
    fprintf(stderr, "%s: write failed: %s\n", argv[0], strerror(rc));
    goto done;
  }

  // close and re-open the file read-only
  (void)close(fd);
  fd = open(path, O_RDONLY);
  if (fd < 0) {
    rc = errno;
    fprintf(stderr, "%s: open failed: %s\n", argv[0], strerror(rc));
    goto done;
  }

  // read this content back in
  char readback[sizeof(CONTENT)] = {0};
  if (read(fd, readback, sizeof(readback) - 1) <
      (ssize_t)sizeof(readback) - 1) {
    rc = errno;
    fprintf(stderr, "%s: read failed: %s\n", argv[0], strerror(rc));
    goto done;
  }

  // delete the file
  (void)close(fd);
  fd = -1;
  if (unlink(path) < 0) {
    rc = errno;
    fprintf(stderr, "%s: unlink failed: %s\n", argv[0], strerror(rc));
    goto done;
  }

done:
  if (fd >= 0)
    (void)close(fd);
  free(path);

  return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}
