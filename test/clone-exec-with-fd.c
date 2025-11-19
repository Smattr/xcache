/// @file
/// @brief Test that unsharing file descriptor tables during `execve` is handled
///
/// When using `clone`/`clone3`, it is possible to ask for the child to share
/// the parent’s file descriptor table using the `CLONE_FILES` flag. This
/// sharing is cut if the child calls `execve`, giving the child an independent
/// file descriptor table, copying whatever was in the shared table at that
/// point. This subtle sequence of interactions is tricky for a tracer to get
/// correct. This program (alone with its sibling, close-fd.c) attempt to create
/// this situation so that we can check whether xcache traces this correctly.

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

static int entry(void *arg) {
  const int fd = (int)(intptr_t)arg;

  char arg0[] = "close-fd";

  char arg1[128] = {0};
  snprintf(arg1, sizeof(arg1), "%d", fd);

  char *const argv[] = {arg0, arg1, NULL};

  execvp(argv[0], argv);
  fprintf(stderr, "execvp failed: %s\n", strerror(errno));

  return EXIT_FAILURE;
}

int main(void) {

  // open a file descriptor that will propagate to the child
  const int fd = open("/dev/null", O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "open /dev/null failed: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  // create a stack for our upcoming child
  enum { STACK_SIZE = 1024 * 1024 };
  char *const stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
  if (stack == MAP_FAILED) {
    fprintf(stderr, "mmap failed: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

#ifdef __hppa__
  char *const stack_top = stack;
#else
  char *const stack_top = stack + STACK_SIZE;
#endif

  // create a child who shares our file descriptor table
  int flags = CLONE_FILES;
  const pid_t child = clone(entry, stack_top, flags, (void *)(intptr_t)fd);
  if (child < 0) {
    fprintf(stderr, "clone failed: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  // wait for the child to finish
  int status;
  const pid_t r = waitpid(child, &status, __WALL);
  if (r < 0) {
    fprintf(stderr, "wait failed: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }
  if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
    fprintf(stderr, "child reported failure: %d\n", status);
    return EXIT_FAILURE;
  }

  // if `execve` correctly unshared our file descriptor table, we should now be
  // able to close our (still open) /dev/null handle
  const int closed = close(fd);
  if (closed < 0) {
    fprintf(stderr, "close failed: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  (void)munmap(stack, STACK_SIZE);

  return EXIT_SUCCESS;
}
