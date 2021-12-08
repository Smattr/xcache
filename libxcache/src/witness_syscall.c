#include "debug.h"
#include "tracee.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/user.h>

static long peek_reg(pid_t pid, size_t offset) {
  return ptrace(PTRACE_PEEKUSER, pid, offset, NULL);
}

#define REG(reg)                                                               \
  offsetof(struct user, regs) + offsetof(struct user_regs_struct, reg)

static int peek_string(char **out, pid_t pid, size_t offset) {

  // read the (tracee) address of this string
  uintptr_t base = (uintptr_t)peek_reg(pid, offset);
  if (base == 0) {
    // NULL pointer
    return EINVAL;
  }

  int rc = 0;

  char *result = NULL;
  size_t size = 0;
  while (true) {

    // FIXME: determine this dynamically
    static const uintptr_t page_size = 4096;

    // we are going to read some data from the tracee, but we want to follow the
    // advice of `man process_vm_readv` and not cross page boundaries
    char chunk[BUFSIZ];
    uintptr_t limit = base - base % page_size + page_size;
    size_t len = limit == 0 ? page_size - base % page_size : limit - base;
    if (limit > sizeof(chunk))
      limit = sizeof(chunk);

    // read some data from the tracee
    struct iovec ours = {.iov_base = chunk, .iov_len = sizeof(chunk)};
    struct iovec theirs = {.iov_base = (void *)base, .iov_len = len};
    ;
    ssize_t r = process_vm_readv(pid, &ours, 1, &theirs, 1, 0);
    if (UNLIKELY(r < 0)) {
      rc = errno;
      goto done;
    }

    // expand our result buffer
    size_t s = (size_t)r;
    assert(s <= sizeof(chunk));
    size_t new_size = size + s;
    char *p = realloc(result, new_size);
    if (UNLIKELY(p == NULL)) {
      rc = ENOMEM;
      goto done;
    }
    result = p;

    // append to our result
    for (size_t i = 0; i < s; ++i) {
      result[size + i] = chunk[i];
      if (chunk[i] == '\0') {
        // found the end of the string
        goto done;
      }
    }

    size = new_size;
  }

done:
  if (UNLIKELY(rc != 0)) {
    free(result);
  } else {
    *out = result;
  }

  return rc;
}

int witness_syscall(tracee_t *tracee) {

  assert(tracee != NULL);

  // retrieve the syscall number
  long nr = peek_reg(tracee->pid, REG(orig_rax));

  // retrieve the result of the syscall
  long ret = peek_reg(tracee->pid, REG(rax));

  switch (nr) {

#ifdef __NR_chdir
  case __NR_chdir: {

    // retrieve the path
    char *path = NULL;
    int rc = peek_string(&path, tracee->pid, REG(rdi));
    if (UNLIKELY(rc != 0))
      return rc;

    rc = witness_chdir(tracee, ret, path);

    free(path);
    return rc;
  }
#endif

  default:
    DEBUG("unrecognised syscall %ld", nr);
  }

  return 0;
}
