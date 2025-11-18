#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

static _Noreturn void child(void) {

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
  exit(EXIT_SUCCESS);
}

int main(void) {

  pid_t pid = syscall(SYS_fork);
  if (pid < 0) {
    fprintf(stderr, "fork failed: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  // are we the child?
  if (pid == 0)
    child();

  return EXIT_SUCCESS;
}
