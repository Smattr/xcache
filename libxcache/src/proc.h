/// @file
/// @brief Abstraction of a (possibly multithreaded) process
///
/// The notion of “process” is somewhat ill-defined in contemporary Linux. In
/// our case, it maps to “thread group” and/or “virtual address space.” That is,
/// we assume PID (not necessarily `pid_t`, which can be used to represent a
/// TID) maps 1-1 with both TGID and address spaces.

#pragma once

#include "../../common/compiler.h"
#include <stddef.h>
#include <sys/types.h>

/// a process being traced
typedef struct {
  pid_t id; ///< process identifier

  /// number of threads homed within this process
  ///
  /// The expectation is that this dropping to 0 represents the termination of
  /// the process.
  size_t ref_count;
} proc_t;

/// create a new process
///
/// The reference count of the created process is set to 1 on success.
///
/// @param pid System identifier of the new process
/// @return A created object on success or `NULL` on out-of-memory
INTERNAL proc_t *proc_new(pid_t pid);

/// take a reference to a process
///
/// This function increments the process’ reference count and returns the same
/// pointer it was passed. It has this type signature to encourage a universal
/// reference-acquisition-and-assignment pattern:
///
///   my_thread->proc = proc_acquire(other_thread->proc);
///
/// @param proc Process to take a reference to
/// @return Same value as the input argument
INTERNAL proc_t *proc_acquire(proc_t *proc);

/// return a reference to a process
///
/// This function decrements the process’ reference count and returns `NULL`. It
/// has this type signature to encourage a universal
/// reference-release-and-null-out pattern:
///
///   my_thread->proc = proc_release(my_thread->proc);
///
/// If the reference count reaches 0 by calling this function, the process is
/// also freed.
///
/// When passed `NULL`, this function is a no-op.
///
/// @param proc Process to return a reference to
/// @return `NULL`
INTERNAL proc_t *proc_release(proc_t *proc);
