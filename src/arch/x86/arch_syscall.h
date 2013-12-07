#ifndef _XCACHE_ARCH_SYSCALL_H_
#define _XCACHE_ARCH_SYSCALL_H_

#include <sys/user.h>

#define REG_SYSNO  orig_eax
#define REG_RESULT eax

#define REG_ARG1   ebx
#define REG_ARG2   ecx
#define REG_ARG3   edx
#define REG_ARG4   esi
#define REG_ARG5   edi
#define REG_ARG6   ebp

#endif
