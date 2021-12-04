#include "debug.h"
#include "macros.h"
#include "tracee.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <xcache/proc.h>

/// Flip this to true when debugging tricky child startup failures. Note this
/// will skip duping stderr, so will make tracing possibly incorrect.
enum { DEBUGGING = false };

#define DEBUG_(args...)                                                        \
  do {                                                                         \
    if (DEBUGGING) {                                                           \
      DEBUG("[tracee] " args);                                                 \
    }                                                                          \
  } while (0)

// The tracee’s actions are coordinated with the tracer. That is, the
// interleaving of what the tracee does below needs to line up with the tracer’s
// actions in `tracee_monitor` with respect to what the tracer is expecting the
// tracee to do. If this is mismatched, it will result in a deadlock where the
// tracer is waiting for the tracee to do some action A while the tracee is
// waiting for the tracer to acknowledge some different action B.

static int exec(tracee_t *tracee) {

  // dup /dev/null over stdin so tracing cannot depend on interactive input
  {
    int in = open("/dev/null", O_RDONLY);
    if (UNLIKELY(in == -1))
      return errno;
    if (UNLIKELY(dup2(in, STDIN_FILENO) == -1)) {
      int r = errno;
      (void)close(in);
      return r;
    }
  }

  // replace our streams with pipes to the parent
  if (UNLIKELY(dup2(tracee->out[1], STDOUT_FILENO) == -1))
    return errno;
  if (!DEBUGGING && UNLIKELY(dup2(tracee->err[1], STDERR_FILENO) == -1))
    return errno;

  // opt-in to being a ptrace tracee
  if (UNLIKELY(ptrace(PTRACE_TRACEME, 0, NULL, NULL) != 0)) {
    int r = errno;
    DEBUG_("failed to opt-in to ptrace: %d", r);
    return r;
  }

  // set no-new-privs so we can install a seccomp filter without CAP_SYS_ADMIN
  if (UNLIKELY(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1)) {
    int r = errno;
    DEBUG_("failed to set no-new-privs: %d", r);
    return r;
  }

  // load a seccomp filter that intercepts relevant syscalls
  static struct sock_filter filter[] = {
      BPF_STMT(BPF_LD + BPF_W + BPF_ABS, offsetof(struct seccomp_data, nr)),

#define TRACE(syscall)                                                         \
  BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_##syscall, 0, 1),                   \
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRACE)

#define IGNORE(syscall)                                                        \
  BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_##syscall, 0, 1),                   \
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW)

      IGNORE(close),
      IGNORE(read),
      IGNORE(write),
      IGNORE(execve),
      IGNORE(fstat),
      IGNORE(poll),
      IGNORE(lseek),
      IGNORE(mmap),
      IGNORE(mprotect),
      IGNORE(munmap),
      IGNORE(brk),
      IGNORE(rt_sigaction),
      IGNORE(rt_sigprocmask),
      IGNORE(rt_sigreturn),
      IGNORE(fcntl),
      IGNORE(ptrace),
#ifdef __NR_set_tid_address
      IGNORE(set_tid_address),
#endif
#ifdef __NR_set_robust_list
      IGNORE(set_robust_list),
#endif
#ifdef __NR_get_robust_list
      IGNORE(get_robust_list),
#endif
      IGNORE(splice),
      IGNORE(tee),
      IGNORE(pipe),
#ifdef __NR_pipe2
      IGNORE(pipe2),
#endif
#ifdef __NR_prlimit64
      IGNORE(prlimit64),
#endif

      TRACE(open),
      TRACE(openat),
      TRACE(access),
      TRACE(getcwd),
#ifdef __NR_arch_prctl
      TRACE(arch_prctl),
#endif

      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),

#undef IGNORE
#undef TRACE
  };
  static const struct sock_fprog prog = {
      .filter = filter,
      .len = sizeof(filter) / sizeof(filter[0]),
  };
  if (UNLIKELY(prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog, 0, 0) == -1)) {
    int r = errno;
    DEBUG_("failed to install seccomp filter: %d", r);
    return r;
  }

  // give our parent an opportunity to attach to us
  {
    int r = raise(SIGSTOP);
    if (UNLIKELY(r != 0)) {
      DEBUG_("failed to SIGSTOP ourselves: %d", r);
      // ignore as there is nothing else we can do
    }
  }

  return xc_proc_exec(tracee->proc);
}

void tracee_exec(tracee_t *tracee) {

  assert(tracee != NULL);
  assert(tracee->proc != NULL);
  assert(tracee->out[0] > 0);
  assert(tracee->err[0] > 0);

  int rc = exec(tracee);

  // we should only reach here on failure
  assert(rc != 0);

  // we failed, so signal this to the parent
  exit(rc);
}
