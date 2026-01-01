/// define SYSENTER_IGNORE for syscalls whose start event is ignored
#ifndef SYSENTER_IGNORE
#error "define SYSENTER_IGNORE before including this file"
#endif

/// define SYSEXIT_IGNORE for syscalls whose end event is ignored
#ifndef SYSEXIT_IGNORE
#error "define SYSEXIT_IGNORE before including this file"
#endif

/// define UNIMPLEMENTED as behaviour for syscalls that should never occur
#ifndef UNIMPLEMENTED
#error "define UNIMPLEMENTED before including this file"
#endif

/// shorthand for ignoring both
#define IGNORE(call) SYSENTER_IGNORE(call) SYSEXIT_IGNORE(call)

#ifndef __NR_read
#error "<sys/syscall.h> seems not to have been #included"
#endif

#ifdef __NR_read
IGNORE(read)
#endif
#ifdef __NR_write
IGNORE(write)
#endif
#ifdef __NR_close
IGNORE(close)
#endif
#ifdef __NR_fstat
IGNORE(fstat)
#endif
#ifdef __NR_poll
IGNORE(poll)
#endif
#ifdef __NR_lseek
IGNORE(lseek)
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
#ifdef __NR_rt_sigaction
IGNORE(rt_sigaction)
#endif
#ifdef __NR_rt_sigprocmask
IGNORE(rt_sigprocmask)
#endif
#ifdef __NR_rt_sigreturn
IGNORE(rt_sigreturn)
#endif
#ifdef __NR_ioctl
SYSEXIT_IGNORE(ioctl) // handled or gave up in sysenter
#endif
#ifdef __NR_pread // pread64 in Linux < 2.6
IGNORE(pread)
#endif
#ifdef __NR_pread64
IGNORE(pread64)
#endif
#ifdef __NR_pwrite // pwrite64 in Linux < 2.6
IGNORE(pwrite)
#endif
#ifdef __NR_pwrite64
IGNORE(pwrite64)
#endif
#ifdef __NR_readv
IGNORE(readv)
#endif
#ifdef __NR_writev
IGNORE(writev)
#endif
#ifdef __NR_access
SYSENTER_IGNORE(access)
#endif
#ifdef __NR_pipe
IGNORE(pipe)
#endif
#ifdef __NR_select
IGNORE(select)
#endif
#ifdef __NR_sched_yield
IGNORE(sched_yield)
#endif
#ifdef __NR_mremap
IGNORE(mremap)
#endif
#ifdef __NR_msync
IGNORE(msync)
#endif
#ifdef __NR_mincore
IGNORE(mincore)
#endif
#ifdef __NR_madvise
IGNORE(madvise)
#endif
#ifdef __NR_dup
IGNORE(dup)
#endif
#ifdef __NR_dup2
IGNORE(dup2)
#endif
#ifdef __NR_getrusage
IGNORE(getrusage)
#endif
// Tell a white lie that we are able to record and replay `getpid` without
// seeing it. We can essentially choose an arbitrary return value for this, so
// no need to cache it.
#ifdef __NR_getpid
IGNORE(getpid)
#endif
#ifdef __NR_execve
SYSEXIT_IGNORE(execve)
#endif
#ifdef __NR_exit
IGNORE(exit)
#endif
#ifdef __NR_wait4
IGNORE(wait4)
#endif
#ifdef __NR_fcntl
IGNORE(fcntl)
#endif
#ifdef __NR_getcwd
IGNORE(getcwd)
#endif
#ifdef __NR_chdir
SYSENTER_IGNORE(chdir)
#endif
#ifdef __NR_unlink
SYSENTER_IGNORE(unlink)
#endif
#ifdef __NR_readlink
SYSENTER_IGNORE(readlink)
#endif
#ifdef __NR_chmod
SYSENTER_IGNORE(chmod)
#endif
// `umask` technically _is_ relevant to the tracer. But it can instead read the
// mode of created files during trace finalisation (`inferior_save`), avoiding
// the need to track umasks.
#ifdef __NR_umask
IGNORE(umask)
#endif
// see note about `getpid`
#ifdef __NR_getppid
IGNORE(getppid)
#endif
#ifdef __NR_sigaltstack
IGNORE(sigaltstack)
#endif
#ifdef __NR_arch_prctl
IGNORE(arch_prctl)
#endif
#ifdef __NR_getpmsg
UNIMPLEMENTED(getpmsg)
#endif
#ifdef __NR_putpmsg
UNIMPLEMENTED(putpmsg)
#endif
#ifdef __NR_afs_syscall
UNIMPLEMENTED(afs_syscall)
#endif
#ifdef __NR_tuxcall
UNIMPLEMENTED(tuxcall)
#endif
#ifdef __NR_security
UNIMPLEMENTED(security)
#endif
#ifdef __NR_futex
IGNORE(futex)
#endif
#ifdef __NR_set_tid_address
IGNORE(set_tid_address)
#endif
// `restart_syscall` is not unimplemented, but we should never see it
#ifdef __NR_restart_syscall
UNIMPLEMENTED(restart_syscall)
#endif
#ifdef __NR_exit_group
IGNORE(exit_group)
#endif
#ifdef __NR_epoll_create
IGNORE(epoll_create)
#endif
#ifdef __NR_vserver
UNIMPLEMENTED(vserver)
#endif
#ifdef __NR_newfstatat
SYSENTER_IGNORE(newfstatat)
#endif
#ifdef __NR_readlinkat
SYSENTER_IGNORE(readlinkat)
#endif
#ifdef __NR_pselect6
IGNORE(pselect6)
#endif
#ifdef __NR_set_robust_list
IGNORE(set_robust_list)
#endif
#ifdef __NR_signalfd
IGNORE(signalfd)
#endif
#ifdef __NR_eventfd
IGNORE(eventfd)
#endif
#ifdef __NR_signalfd4
IGNORE(signalfd4)
#endif
#ifdef __NR_eventfd2
IGNORE(eventfd2)
#endif
#ifdef __NR_epoll_create1
IGNORE(epoll_create1)
#endif
#ifdef __NR_dup3
IGNORE(dup3)
#endif
#ifdef __NR_pipe2
IGNORE(pipe2)
#endif
#ifdef __NR_preadv
IGNORE(preadv)
#endif
#ifdef __NR_pwritev
IGNORE(pwritev)
#endif
#ifdef __NR_prlimit64
IGNORE(prlimit64)
#endif
#ifdef __NR_getrandom
SYSENTER_IGNORE(getrandom)
#endif
#ifdef __NR_memfd_create
IGNORE(memfd_create)
#endif
#ifdef __NR_preadv2
IGNORE(preadv2)
#endif
#ifdef __NR_pwritev2
IGNORE(pwritev2)
#endif
#ifdef __NR_pidfd_open
SYSENTER_IGNORE(pidfd_open)
#endif
#ifdef __NR_rseq
IGNORE(rseq)
#endif
#ifdef __NR_faccessat2
SYSENTER_IGNORE(faccessat2)
#endif
