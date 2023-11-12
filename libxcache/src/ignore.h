/// define SYSENTER_IGNORE for syscalls whose start event is ignored
#ifndef SYSENTER_IGNORE
#error "define SYSENTER_IGNORE before including this file"
#endif

/// define SYSEXIT_IGNORE for syscalls whose end event is ignored
#ifndef SYSEXIT_IGNORE
#error "define SYSEXIT_IGNORE before including this file"
#endif

/// shorthand for ignoring both
#define IGNORE(call) SYSENTER_IGNORE(call) SYSEXIT_IGNORE(call)

#ifndef __NR_read
#error "<sys/syscall.h> seems not to have been #included"
#endif

#ifdef __NR_close
SYSENTER_IGNORE(close)
#endif
#ifdef __NR_mmap
IGNORE(mmap)
#endif
#ifdef __NR_brk
IGNORE(brk)
#endif
#ifdef __NR_access
SYSENTER_IGNORE(access)
#endif
#ifdef __NR_execve
SYSEXIT_IGNORE(execve)
#endif
#ifdef __NR_chdir
SYSENTER_IGNORE(chdir)
#endif
#ifdef __NR_arch_prctl
IGNORE(arch_prctl)
#endif
#ifdef __NR_openat
SYSENTER_IGNORE(openat)
#endif
#ifdef __NR_newfstatat
SYSENTER_IGNORE(newfstatat)
#endif
