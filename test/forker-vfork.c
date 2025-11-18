/// @file
/// @brief Version of forker.c that uses the `vfork` syscall instead of `clone`

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

static _Noreturn void child(void) {

  // due to the limitations of vfork, we cannot really do much here

  _Exit(EXIT_SUCCESS);
}

int main(void) {

  pid_t pid = syscall(SYS_vfork);
  if (pid < 0) {
    fprintf(stderr, "vfork failed: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  // are we the child?
  if (pid == 0)
    child();

  FILE *f = fopen("foo", "w");
  if (f == NULL) {
    fprintf(stderr, "fopen failed\n");
    exit(EXIT_FAILURE);
  }

  if (fprintf(f, "hello world") < 0) {
    fprintf(stderr, "failed to write content\n");
    exit(EXIT_FAILURE);
  }

  (void)fclose(f);
  return EXIT_SUCCESS;
}
