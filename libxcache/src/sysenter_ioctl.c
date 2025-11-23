#include "../../common/proccall.h"
#include "debug.h"
#include "inferior_t.h"
#include "input_t.h"
#include "peek.h"
#include "syscall.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <xcache/record.h>

/// handle the tracee having signalled us with `CALL_SYSCONF`
static int handle_sysconf(inferior_t *inf, thread_t *thread) {
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

int sysenter_ioctl(inferior_t *inf, thread_t *thread) {

  assert(inf != NULL);
  assert(thread != NULL);

  int rc = 0;

  // extract the file descriptor
  const int fd = (int)peek_syscall_arg(thread, 1);

  // extract the call number
  const long callno = peek_syscall_arg(thread, 2);

  // ioctls we can reasonably ignore
  const struct {
    const char *name;
    long value;
  } SAFE[] = {
#define X(v) {#v, v}
      X(TCGETS),
#undef X
  };
  for (size_t i = 0; i < sizeof(SAFE) / sizeof(SAFE[i]); ++i) {
    if (callno == SAFE[i].value) {
      DEBUG("TID %ld, ignoring safe ioctl(%d, %s, …)", (long)thread->id, fd,
            SAFE[i].name);
      goto done;
    }
  }

  // any ioctl except a communication from the spy is unsupported
  if (ERROR(fd != XCACHE_FILENO)) {
    DEBUG("TID %ld, ioctl(%d, %ld, …)", (long)thread->id, fd, callno);
    if (thread->ignoring) {
      DEBUG("ignoring ioctl«%lu» on spy’s instruction",
            (unsigned long)__NR_ioctl);
      goto done;
    }
    rc = ECHILD;
    goto done;
  }

  DEBUG("TID %ld, ioctl(%d (XCACHE_FILENO), 0x%lx (%s), …)", (long)thread->id,
        fd, callno, callno_to_str(callno));

  // dispatch call
  switch (callno) {
  case CALL_OFF:
    assert(!thread->ignoring && "duplicate monitor disable messages");
    thread->ignoring = true;
    break;

  case CALL_ON:
    assert(thread->ignoring && "duplicate monitor enable messages");
    thread->ignoring = false;
    break;

  case CALL_SYSCONF:
    if (ERROR((rc = handle_sysconf(inf, thread))))
      goto done;
    break;

  default:
    DEBUG("unrecognised message from libxcache-spy: %ld", callno);
    assert(callno == CALL_OFF || callno == CALL_ON || callno == CALL_SYSCONF);
  }

done:
  return rc;
}
