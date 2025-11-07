#pragma once

#include <linux/version.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

/// is this `wait` status an `exec`?
static inline bool is_exec(int status) {
  if (!WIFSTOPPED(status))
    return false;
  if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXEC << 8)))
    return true;
  return false;
}

/// is this `wait` status a ptrace-observed exit?
static inline bool is_exit(int status) {
  if (!WIFSTOPPED(status))
    return false;
  if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXIT << 8)))
    return true;
  return false;
}

/// is this `wait` status a fork or cousin thereof?
static inline bool is_fork(int status) {
  if (!WIFSTOPPED(status))
    return false;
  if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_CLONE << 8)))
    return true;
  if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_FORK << 8)))
    return true;
  if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK << 8)))
    return true;
  return false;
}

/// is this `wait` status a seccomp stop?
static inline bool is_seccomp(int status) {
  if (!WIFSTOPPED(status))
    return false;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)
  if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_SECCOMP << 8)))
    return true;
#endif
  return false;
}

/// is this `wait` status a syscall stop?
static inline bool is_syscall(int status) {
  if (!WIFSTOPPED(status))
    return false;
  if (status >> 8 == (SIGTRAP | 0x80))
    return true;
  return false;
}

/// is this `wait` status the end of `vfork`?
static inline bool is_vfork_done(int status) {
  if (!WIFSTOPPED(status))
    return false;
  if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK_DONE << 8)))
    return true;
  return false;
}

/// is this a traditional `WIFSTOPPED` (a signal)?
static inline bool is_signal(int status) {
  if (!WIFSTOPPED(status))
    return false;
  if (is_exec(status))
    return false;
  if (is_exit(status))
    return false;
  if (is_fork(status))
    return false;
  if (is_seccomp(status))
    return false;
  if (is_syscall(status))
    return false;
  if (is_vfork_done(status))
    return false;
  return true;
}
