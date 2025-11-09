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

/// handle start of `ioctl`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysenter_ioctl(inferior_t *inf, thread_t *thread);

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
/// \param inf Tracee to which the target belongs
/// \param thread Caller thread
/// \return 0 on success or an errno on failure
INTERNAL int sysexit_chdir(inferior_t *inf, thread_t *thread);

/// handle end of `close`
///
/// @param inf Tracee to which the target belongs
/// @param thread Caller thread
/// @return 0 on success or an errno on failure
INTERNAL int sysexit_close(inferior_t *inf, thread_t *thread);

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

/// convert a syscall number to its name
///
/// @param number Syscall to lookup
/// @return The name of the syscall of "<unknown>" if there was no match
INTERNAL const char *syscall_to_str(unsigned long number);
