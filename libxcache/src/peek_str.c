#include "debug.h"
#include "page_size.h"
#include "peek.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>

int peek_str(char **out, const proc_t *proc, uintptr_t addr) {

  assert(out != NULL);

  int rc = 0;

  const uintptr_t pagesize = (uintptr_t)page_size();

  char *result = NULL;
  size_t size = 0;
  for (uintptr_t base = addr;;) {

    // we are going to read some data from the tracee, but we want to follow the
    // advice of `man process_vm_readv` and not cross page boundaries
    char chunk[BUFSIZ];
    uintptr_t limit = base - base % pagesize + pagesize;
    size_t len = limit == 0 ? pagesize - base % pagesize : limit - base;
    if (len > sizeof(chunk))
      len = sizeof(chunk);

    // read some data from the tracee
    struct iovec ours = {.iov_base = chunk, .iov_len = sizeof(chunk)};
    struct iovec theirs = {.iov_base = (void *)base, .iov_len = len};
    ssize_t r = process_vm_readv(proc->id, &ours, 1, &theirs, 1, 0);
    if (ERROR(r < 0)) {
      // if the process’ address space was torn down while we were trying to
      // read from it, treat this as unsupported
      if (errno == ESRCH) {
        rc = ECHILD;
      } else {
        rc = errno;
      }
      goto done;
    }

    // expand our result buffer
    size_t s = (size_t)r;
    assert(s <= sizeof(chunk));
    size_t new_size = size + s;
    char *p = realloc(result, new_size);
    if (ERROR(p == NULL)) {
      rc = ENOMEM;
      goto done;
    }
    result = p;

    // append to our result
    for (size_t i = 0; i < s; ++i) {
      result[size + i] = chunk[i];
      if (chunk[i] == '\0') {
        // found the end of the string
        goto break2;
      }
    }

    size += new_size;
    base += s;
  }
break2:

  *out = result;
  result = NULL;

done:
  free(result);

  return rc;
}
