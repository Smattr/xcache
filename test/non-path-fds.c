/// @file
/// @brief Create a variety of in-memory file descriptors
///
/// Xcache pays attention to file descriptors used by the tracee, as these
/// represent external files the tracee is reading and writing. However, Linux
/// has a number of ways to create file descriptors that do not refer to files.
/// This program explores a number of these ways, performing operations on the
/// (non-file) file descriptors that should probe whether xcache understands to
/// ignore such operations.

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/signalfd.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(__attribute__((unused)) int argc, char **argv) {

#ifdef AT_EMPTY_PATH
  // create a pipe and `access`
  {
    int fd[2];
    if (pipe(fd) < 0) {
      fprintf(stderr, "%s: pipe failed: %s\n", argv[0], strerror(errno));
      return EXIT_FAILURE;
    }

    (void)faccessat(fd[0], "", F_OK, AT_EMPTY_PATH);

    (void)close(fd[0]);
    (void)close(fd[1]);
  }

  // create a pipe a different way and access it
  {
    int fd[2];
    if (pipe2(fd, O_CLOEXEC) < 0) {
      fprintf(stderr, "%s: pipe failed: %s\n", argv[0], strerror(errno));
      return EXIT_FAILURE;
    }

    (void)faccessat(fd[0], "", F_OK, AT_EMPTY_PATH);

    (void)close(fd[0]);
    (void)close(fd[1]);
  }

  // create an epoll descriptor and access it
  {
    const int fd = epoll_create(1);
    if (fd < 0) {
      fprintf(stderr, "%s: epoll_create failed: %s\n", argv[0],
              strerror(errno));
      return EXIT_FAILURE;
    }

    (void)faccessat(fd, "", F_OK, AT_EMPTY_PATH);

    (void)close(fd);
  }

  // create an epoll descriptor a different way and access it
  {
    const int fd = epoll_create1(0);
    if (fd < 0) {
      fprintf(stderr, "%s: epoll_create1 failed: %s\n", argv[0],
              strerror(errno));
      return EXIT_FAILURE;
    }

    (void)faccessat(fd, "", F_OK, AT_EMPTY_PATH);

    (void)close(fd);
  }

  // create a signal file descriptor and access it
  {
    const int fd = signalfd(-1, &(sigset_t){0}, 0);
    if (fd < 0) {
      fprintf(stderr, "%s: signalfd failed: %s\n", argv[0], strerror(errno));
      return EXIT_FAILURE;
    }

    (void)faccessat(fd, "", F_OK, AT_EMPTY_PATH);

    (void)close(fd);
  }

  // create an event file descriptor and access it
  {
    const int fd = eventfd(0, 0);
    if (fd < 0) {
      fprintf(stderr, "%s: eventfd failed: %s\n", argv[0], strerror(errno));
      return EXIT_FAILURE;
    }

    (void)faccessat(fd, "", F_OK, AT_EMPTY_PATH);

    (void)close(fd);
  }

  // create a in-memory file and access it
  {
    const int fd = memfd_create("foo", 0);
    if (fd < 0) {
      fprintf(stderr, "%s: memfd_create failed: %s\n", argv[0],
              strerror(errno));
      return EXIT_FAILURE;
    }

    (void)faccessat(fd, "", F_OK, AT_EMPTY_PATH);

    (void)close(fd);
  }

#ifdef __NR_pidfd_open
  // create a process file descriptor and access it
  {
    const int fd = syscall(SYS_pidfd_open, getpid(), 0);
    if (fd < 0) {
      fprintf(stderr, "%s: pidfd_open failed: %s\n", argv[0], strerror(errno));
      return EXIT_FAILURE;
    }

    (void)faccessat(fd, "", F_OK, AT_EMPTY_PATH);

    (void)close(fd);
  }
#endif

#endif

  return EXIT_SUCCESS;
}
