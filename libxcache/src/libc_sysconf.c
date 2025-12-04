#include "debug.h"
#include "inferior_t.h"
#include "input_t.h"
#include "peek.h"
#include "syscall.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>

/// handle the tracee having signalled us with `CALL_SYSCONF`
int libc_sysconf(inferior_t *inf, thread_t *thread) {
  assert(inf != NULL);
  assert(thread != NULL);

  input_t input = {0};
  int rc = 0;

  const long arg = peek_syscall_arg(thread, 3);

  const struct {
    const char *name;
    long value;
  } KNOWN[] = {
  // this array needs to be a superset of
  // ../../libxcache-spy/src/init.c::known_sysconf’s cases
#define X(v) {#v, v}
      X(_SC_PAGESIZE),
      X(_SC_PHYS_PAGES),
#undef X
  };
  const char *name = NULL;
  for (size_t i = 0; i < sizeof(KNOWN) / sizeof(KNOWN[0]); ++i) {
    if (arg != KNOWN[i].value)
      continue;
    name = KNOWN[i].name;
    break;
  }

  DEBUG("TID %ld called sysconf(%ld /* %s */)", (long)thread->id, arg,
        name == NULL ? "unknown" : name);

  // if we do not know this sysconf, consider this action unsupported
  if (ERROR(name == NULL)) {
    rc = ECHILD;
    goto done;
  }

  // we assume the tracee sees the same `sysconf` results we do
  if (ERROR((rc = input_new_sysconf(&input, (int)arg))))
    goto done;
  if (ERROR((rc = inferior_input_new(inf, input))))
    goto done;
  input = (input_t){0};

done:
  input_free(input);

  return rc;
}
