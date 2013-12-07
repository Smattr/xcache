#ifndef _XCACHE_ARCH_SYSCALL_H_
#define _XCACHE_ARCH_SYSCALL_H_

#include <sys/user.h>

#define REG_SYSNO  orig_rax
#define REG_RESULT rax

#define REG_ARG1   rdi
#define REG_ARG2   rsi
#define REG_ARG3   rdx
#define REG_ARG4   r10
#define REG_ARG5   r8
#define REG_ARG6   r9

#endif
