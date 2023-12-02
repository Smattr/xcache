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

#ifdef __NR_read
IGNORE(read)
#endif
#ifdef __NR_close
SYSENTER_IGNORE(close)
#endif
#ifdef __NR_mmap
IGNORE(mmap)
#endif
#ifdef __NR_mprotect
IGNORE(mprotect)
#endif
#ifdef __NR_munmap
IGNORE(munmap)
#endif
#ifdef __NR_brk
IGNORE(brk)
#endif
#ifdef __NR_ioctl
SYSEXIT_IGNORE(ioctl) // handled or gave up in sysenter
#endif
#ifdef __NR_pread64
IGNORE(pread64)
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
#ifdef __NR_set_tid_address
IGNORE(set_tid_address)
#endif
#ifdef __NR_exit_group
IGNORE(exit_group)
#endif
#ifdef __NR_openat
SYSENTER_IGNORE(openat)
#endif
#ifdef __NR_newfstatat
SYSENTER_IGNORE(newfstatat)
#endif
#ifdef __NR_prlimit64
IGNORE(prlimit64)
#endif
#ifdef __NR_set_robust_list
IGNORE(set_robust_list)
#endif
#ifdef __NR_rseq
IGNORE(rseq)
#endif
