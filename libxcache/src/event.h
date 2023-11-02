#pragma once

#include <signal.h>
#include <stdbool.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

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
  if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_SECCOMP << 8)))
    return true;
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
