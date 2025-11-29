/// @file
/// @brief Clone in a configurable way, then do something relevant in the child
///
/// See test.py::test_ld_preload_in_child for the purpose of this.

#define _GNU_SOURCE
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

static int child_main(__attribute__((unused)) void *arg) {

  // do something that we expect xcache to record via libxcache-spy
  const long pagesize = sysconf(_SC_PAGESIZE);

  printf("page size: %ld\n", pagesize);

  return 0;
}

int main(int argc, char **argv) {

  // allocate a stack for the child
  const size_t STACK_SIZE = 1024 * 1024;
  char *const stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
  if (stack == MAP_FAILED) {
    fprintf(stderr, "%s: mmap failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

#ifdef __hppa__
  char *const stack_top = stack;
#else
  char *const stack_top = stack + STACK_SIZE;
#endif

  // decide flags to configure the child
  int flags = CLONE_PTRACE;
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "CLONE_FILES") == 0)
      flags |= CLONE_FILES;
    if (strcmp(argv[i], "CLONE_FS") == 0)
      flags |= CLONE_FS;
    if (strcmp(argv[i], "CLONE_VM") == 0)
      flags |= CLONE_VM;
  }

  // start the child
  const pid_t pid = clone(child_main, stack_top, flags, NULL);
  if (pid < 0) {
    fprintf(stderr, "%s: clone failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  // wait for the child to do its work
  if (waitpid(pid, NULL, __WALL) < 0) {
    fprintf(stderr, "%s: waitpid failed: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  (void)munmap(stack, STACK_SIZE);

  return 0;
}
