#include "debug.h"
#include "peek.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/ptrace.h>

static size_t min(size_t a, size_t b) { return a < b ? a : b; }

int peek_data(void *out, size_t size, const proc_t *proc, uintptr_t addr) {
  assert(out != NULL || size == 0);
  assert(proc != NULL);

  int rc = 0;

  char *dst = out;
  for (size_t offset = 0; offset < size;) {

    // Read a word out of the tracee. We could use `process_vm_readv` instead
    // for this, but we assume the length being read is relatively short.
    errno = 0;
    const long chunk =
        ptrace(PTRACE_PEEKDATA, proc->id, (void *)(addr + offset), NULL);
    if (ERROR(errno != 0)) {
      // if the process’ address space was torn down while we were trying to
      // read from it, treat this as unsupported
      if (errno == ESRCH) {
        rc = ECHILD;
      } else {
        rc = errno;
      }
      goto done;
    }

    // copy this word (or a slice of it) into the output
    const size_t len = min(sizeof(chunk), size - offset);
    memcpy(dst, &chunk, len);
    dst += len;
    offset += sizeof(chunk);
  }

done:
  return rc;
}
