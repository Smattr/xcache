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

#ifdef __NR_brk
IGNORE(brk)
#endif
#ifdef __NR_execve
SYSEXIT_IGNORE(execve)
#endif
#ifdef __NR_chdir
SYSENTER_IGNORE(chdir)
#endif
