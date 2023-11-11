#pragma once

#include "../../common/compiler.h"
#include <stddef.h>
#include <stdint.h>
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

/// read the syscall number a child is in the midst of calling
static inline unsigned long peek_syscall_no(pid_t pid) {
  return (unsigned long)peek_reg(pid, REG(orig_rax));
}

/// read the syscall return value at sysexit
static inline long peek_ret(pid_t pid) { return peek_reg(pid, REG(rax)); }

/// read the errno value at sysexit
static inline int peek_errno(pid_t pid) {
  const long ret = peek_ret(pid);
  if (ret >= 0)
    return 0;
  return (int)-ret;
}

/** read a NUL-terminated string out of a child’s address space
 *
 * \param out [out] Read string on success
 * \param pid Process to read from
 * \param addr Address to read from
 */
INTERNAL int peek_str(char **out, pid_t pid, uintptr_t addr);
