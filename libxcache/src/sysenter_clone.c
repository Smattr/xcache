#include "../../common/compiler.h"
#include "debug.h"
#include "peek.h"
#include "syscall.h"
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <linux/sched.h>
#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>

/// convert `clone` flags into a readable string representation
static char *flags_to_str(uint64_t flags) {

  char *buffer = NULL;
  size_t buffer_size = 0;
  FILE *stream = NULL;
  char *ret = NULL;

  stream = open_memstream(&buffer, &buffer_size);
  if (stream == NULL)
    goto done;

  const struct {
    uint64_t value;
    const char *name;
  } FLAGS[] = {
#define X(f) {f, #f}
      X(CLONE_CHILD_CLEARTID),
      X(CLONE_CHILD_SETTID),
#ifdef CLONE_CLEAR_SIGHAND
      X(CLONE_CLEAR_SIGHAND),
#endif
      X(CLONE_DETACHED),
      X(CLONE_FILES),
      X(CLONE_FS),
#ifdef CLONE_INTO_CGROUP
      X(CLONE_INTO_CGROUP),
#endif
      X(CLONE_IO),
#ifdef CLONE_NEWCGROUP
      X(CLONE_NEWCGROUP),
#endif
      X(CLONE_NEWIPC),
      X(CLONE_NEWNET),
      X(CLONE_NEWNS),
      X(CLONE_NEWPID),
      X(CLONE_NEWUSER),
      X(CLONE_NEWUTS),
      X(CLONE_PARENT),
      X(CLONE_PARENT_SETTID),
#ifdef CLONE_PIDFD
      X(CLONE_PIDFD),
#endif
      X(CLONE_PTRACE),
      X(CLONE_SETTLS),
      X(CLONE_SIGHAND),
      X(CLONE_SYSVSEM),
      X(CLONE_THREAD),
      X(CLONE_UNTRACED),
      X(CLONE_VFORK),
      X(CLONE_VM),
#undef X
  };

  const char *separator = "";
  for (size_t i = 0; i < sizeof(FLAGS) / sizeof(FLAGS[0]); ++i) {
    if (!(flags & FLAGS[i].value))
      continue;
    if (fprintf(stream, "%s%s", separator, FLAGS[i].name) < 0)
      goto done;
    separator = "|";
    flags &= ~FLAGS[i].value;
  }

  if (flags != 0) {
    if (fprintf(stream, "%s%" PRIu64, separator, flags) < 0)
      goto done;
  }

  (void)fclose(stream);
  stream = NULL;
  ret = buffer;
  buffer = NULL;

done:
  if (stream != NULL)
    (void)fclose(stream);
  free(buffer);

  return ret;
}

/// common inner handling logic
static void core(inferior_t *inf, thread_t *thread, uint64_t flags) {

  assert(inf != NULL);
  assert(thread != NULL);

  // Extract flags. We do not need to worry about `CLONE_VM` because (1) we do
  // all VM reads via PID and (2) `CLONE_THREAD` implies `CLONE_VM`.
  assert(!thread->clone_flags.set);
  thread->clone_flags = (clone_flags_t){.set = true,
                                        .clone_fs = flags & CLONE_FS,
                                        .clone_thread = flags & CLONE_THREAD};
}

int sysenter_clone(inferior_t *inf, thread_t *thread) {

  assert(inf != NULL);
  assert(thread != NULL);

  int rc = 0;

  // We do not actually need to record `clone` itself. But as far as I can tell,
  // there is no other way than inspecting the `clone` arguments to tell whether
  // the new child will share FDs/CWD/VM with the parent.

  assert(!thread->clone_flags.set &&
         "thread called `clone` while another clone was still in progress");

#ifndef __x86_64__
#error "the following assumes the x86-64 prototype of clone"
#endif

  // extract the flags
  const uint64_t flags = peek_syscall_arg(thread, 1);

  if (UNLIKELY(xc_debug != NULL)) {
    char *const flags_str = flags_to_str(flags & ~0xff);
    DEBUG("TID %ld, clone(%s | /* exit signal = */ %" PRIu64 ", …)",
          (long)thread->id, flags_str == NULL ? "<oom>" : flags_str,
          flags & 0xff);
    free(flags_str);
  }

  core(inf, thread, flags & ~0xff);

  return rc;
}

int sysenter_clone3(inferior_t *inf, thread_t *thread) {

  assert(inf != NULL);
  assert(thread != NULL);

  int rc = 0;

  // We do not actually need to record `clone3` itself. But as far as I can
  // tell, there is no other way than inspecting the `clone` arguments to tell
  // whether the new child will share FDs/CWD/VM with the parent.

  assert(!thread->clone_flags.set &&
         "thread called `clone3` while another clone was still in progress");

  // extract `cl_args`
  const uintptr_t cl_args = (uintptr_t)peek_syscall_arg(thread, 1);

  // Extract the claimed size of `cl_args`. My reading of `clone(2)` is that it
  // may be valid to pass `size < sizeof(struct clone_args)`, implying that you
  // want defaults for fields beyond this size. E.g. `size == 0`.
  const size_t size = (size_t)peek_syscall_arg(thread, 2);

  // from this, we should be able to derive the flags
  uint64_t flags = 0;
  if (cl_args != 0) {
    if (size >= offsetof(struct clone_args, flags) + sizeof(uint64_t)) {
      errno = 0;
      flags =
          (uint64_t)ptrace(PTRACE_PEEKDATA, thread->id, (void *)cl_args, NULL);
      if (ERROR(errno != 0)) {
        rc = errno;
        if (rc == EFAULT || rc == EIO)
          rc = ECHILD;
        goto done;
      }
    }
  }

  if (UNLIKELY(xc_debug != NULL)) {
    char *const flags_str = flags_to_str(flags);
    DEBUG("TID %ld, clone3(&(struct clone_args){.flags = %s, …}, %zu)",
          (long)thread->id, flags_str == NULL ? "<oom>" : flags_str, size);
    free(flags_str);
  }

  core(inf, thread, flags);

done:
  return rc;
}

int sysenter_fork(inferior_t *inf, thread_t *thread) {

  assert(inf != NULL);
  assert(thread != NULL);

  assert(!thread->clone_flags.set &&
         "thread called `fork` while another clone was still in progress");

  // One might think it is possible to ignore `fork` and just infer its known
  // behaviour when seeing a `PTRACE_EVENT_FORK`. However, it turns out that a
  // `clone`/`clone3` with the set of flags that correspond to `fork` causes a
  // `PTRACE_EVENT_FORK` not a `PTRACE_EVENT_CLONE`. That is, when seeing a
  // `PTRACE_EVENT_FORK` we do not know it was definitely triggered by a `fork`.
  // It would be possible to infer `flags == 0` when seeing a
  // `PTRACE_EVENT_FORK` with no saved flags. But this seems like a risky design
  // that would easily mask xcache bugs.

  // `fork` unshares everything
  const uint64_t flags = 0;

  core(inf, thread, flags);

  return 0;
}

int sysenter_vfork(inferior_t *inf, thread_t *thread) {

  assert(inf != NULL);
  assert(thread != NULL);

  assert(!thread->clone_flags.set &&
         "thread called `vfork` while another clone was still in progress");

  // One might think it is possible to ignore `vfork` and just infer its known
  // behaviour when seeing a `PTRACE_EVENT_VFORK`. However, it turns out that a
  // `clone`/`clone3` with the set of flags that correspond to `vfork` causes a
  // `PTRACE_EVENT_VFORK` not a `PTRACE_EVENT_CLONE`. That is, when seeing a
  // `PTRACE_EVENT_VFORK` we do not know it was definitely triggered by a
  // `vfork`. It would be possible to infer `flags == CLONE_THREAD` when seeing
  // a `PTRACE_EVENT_VFORK` with no saved flags. But this seems like a risky
  // design that would easily mask xcache bugs.

  // `vfork` unshares everything except the virtual address space
  const uint64_t flags = CLONE_THREAD;

  core(inf, thread, flags);

  return 0;
}
