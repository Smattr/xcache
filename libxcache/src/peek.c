#include "peek.h"
#include "debug.h"
#include "macros.h"
#include "page_size.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>

int peek_string(char **out, pid_t pid, size_t offset) {

  assert(out != NULL);

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

    const uintptr_t pagesize = (uintptr_t)page_size();

    // we are going to read some data from the tracee, but we want to follow the
    // advice of `man process_vm_readv` and not cross page boundaries
    char chunk[BUFSIZ];
    uintptr_t limit = base - base % pagesize + pagesize;
    size_t len = limit == 0 ? pagesize - base % pagesize : limit - base;
    if (limit > sizeof(chunk))
      limit = sizeof(chunk);

    // read some data from the tracee
    struct iovec ours = {.iov_base = chunk, .iov_len = sizeof(chunk)};
    struct iovec theirs = {.iov_base = (void *)base, .iov_len = len};
    ;
    ssize_t r = process_vm_readv(pid, &ours, 1, &theirs, 1, 0);
    if (ERROR(r < 0)) {
      rc = errno;
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
