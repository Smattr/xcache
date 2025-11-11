#pragma once

#include "../../common/compiler.h"
#include "thread_t.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>

/// read a register from a traced child
static inline long peek_reg(const thread_t *thread, size_t offset) {
  return ptrace(PTRACE_PEEKUSER, thread->id, offset, NULL);
}

/// helper for translating to register offsets
#define REG(reg)                                                               \
  offsetof(struct user, regs) + offsetof(struct user_regs_struct, reg)

/// get syscall arguments (see `man -s2 syscall`)
static inline long peek_syscall_arg(const thread_t *thread, unsigned arg) {
  assert(arg > 0);
  assert(arg < 7);
  const size_t offsets[] = {REG(rdi), REG(rsi), REG(rdx),
                            REG(r10), REG(r8),  REG(r9)};
  return peek_reg(thread, offsets[arg - 1]);
}

/// read the syscall number a child is in the midst of calling
static inline unsigned long peek_syscall_no(const thread_t *thread) {
  return (unsigned long)peek_reg(thread, REG(orig_rax));
}

/// read the syscall return value at sysexit
static inline long peek_ret(const thread_t *thread) {
  return peek_reg(thread, REG(rax));
}

/// read the errno value at sysexit
static inline int peek_errno(const thread_t *thread) {
  const long ret = peek_ret(thread);
  if (ret >= 0)
    return 0;
  return (int)-ret;
}

/// read a NUL-terminated string out of a child’s address space
///
/// @param out [out] Read string on success
/// @param proc Process to read from
/// @param addr Address to read from
/// @return 0 on success or an errno on failure
INTERNAL int peek_str(char **out, const proc_t *proc, uintptr_t addr);
