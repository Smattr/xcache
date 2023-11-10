/// define IGNORE to an expansion for universally ignored syscalls
#ifndef IGNORE
#error "defined IGNORE before including this file"
#endif

/// optionally define SYSENTER_IGNORE for syscalls whose start event is ignored
#ifndef SYSENTER_IGNORE
#define SYSENTER_IGNORE(no) IGNORE(no)
#endif

#ifndef __NR_read
#error "<sys/syscall.h> seems not to have been #included"
#endif

#ifdef __NR_chdir
SYSENTER_IGNORE(chdir)
#endif
