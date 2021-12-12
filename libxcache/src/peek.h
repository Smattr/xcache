#pragma once

#include "macros.h"
#include <stddef.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>

/// read a register from a traced child
static inline long peek_reg(pid_t pid, size_t offset) {
  return ptrace(PTRACE_PEEKUSER, pid, offset, NULL);
}

/// helper for translating to register offsets
#define REG(reg)                                                               \
  offsetof(struct user, regs) + offsetof(struct user_regs_struct, reg)

/// read a NUL-terminated string out of a child’s address space
INTERNAL int peek_string(char **out, pid_t pid, size_t offset);
