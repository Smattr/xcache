#pragma once

#include "../../common/compiler.h"
#include "inferior_t.h"
#include "thread_t.h"

/// handle start of a syscall
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysenter(inferior_t *inf, thread_t *thread);

/// handle start of `execve`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysenter_execve(inferior_t *inf, thread_t *thread);

/// handle start of `fork`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysenter_fork(inferior_t *inf, thread_t *thread);

/// handle start of `ioctl`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysenter_ioctl(inferior_t *inf, thread_t *thread);

/// handle start of `clone`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysenter_clone(inferior_t *inf, thread_t *thread);

/// handle start of `clone3`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysenter_clone3(inferior_t *inf, thread_t *thread);

/// handle start of `openat`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysenter_openat(inferior_t *inf, thread_t *thread);

/// handle start of `vfork`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysenter_vfork(inferior_t *inf, thread_t *thread);

/// handle end of a syscall
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysexit(inferior_t *inf, thread_t *thread);

/// handle end of `access`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysexit_access(inferior_t *inf, thread_t *thread);

/// handle end of `chdir`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysexit_chdir(inferior_t *inf, thread_t *thread);

/// handle end of `chmod`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysexit_chmod(inferior_t *inf, thread_t *thread);

/// handle end of `clone`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysexit_clone(inferior_t *inf, thread_t *thread);

/// handle end of `clone3`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysexit_clone3(inferior_t *inf, thread_t *thread);

/// handle end of `faccessat2`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysexit_faccessat2(inferior_t *inf, thread_t *thread);

/// handle end of `fork`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysexit_fork(inferior_t *inf, thread_t *thread);

/// handle end of `getrandom`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysexit_getrandom(inferior_t *inf, thread_t *thread);

/// handle end of `newfstatat`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysexit_newfstatat(inferior_t *inf, thread_t *thread);

/// handle end of `openat`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysexit_openat(inferior_t *inf, thread_t *thread);

/// handle end of `pidfd_open`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysexit_pidfd_open(inferior_t *inf, thread_t *thread);

/// handle end of `readlink`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysexit_readlink(inferior_t *inf, thread_t *thread);

/// handle end of `readlinkat`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysexit_readlinkat(inferior_t *inf, thread_t *thread);

/// handle end of `vfork`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysexit_vfork(inferior_t *inf, thread_t *thread);

/// convert a syscall number to its name
///
/// @param number Syscall to lookup
/// @return The name of the syscall of "<unknown>" if there was no match
INTERNAL const char *syscall_to_str(unsigned long number);

/// handle call to `clearenv`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int libc_clearenv(inferior_t *inf, thread_t *thread);

/// handle call to `getenv`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int libc_getenv(inferior_t *inf, thread_t *thread);

/// handle call to `putenv`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int libc_putenv(inferior_t *inf, thread_t *thread);

/// handle call to `setenv`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int libc_setenv(inferior_t *inf, thread_t *thread);

/// handle call to `sysconf`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int libc_sysconf(inferior_t *inf, thread_t *thread);

/// handle call to `unsetenv`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int libc_unsetenv(inferior_t *inf, thread_t *thread);
