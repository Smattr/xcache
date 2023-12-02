#pragma once

#include "../../common/compiler.h"
#include "proc_t.h"

/** handle start of a syscall
 *
 * \param proc Process that made this syscall
 * \return 0 on success or an errno on failure
 */
INTERNAL int sysenter(proc_t *proc);

/** handle start of `execve`
 *
 * \param proc Process that made this syscall
 * \return 0 on success or an errno on failure
 */
INTERNAL int sysenter_execve(proc_t *proc);

/** handle start of `ioctl`
 *
 * \param proc Process that made this syscall
 * \return 0 on success or an errno on failure
 */
INTERNAL int sysenter_ioctl(proc_t *proc);

/** handle end of a syscall
 *
 * \param proc Process that made this syscall
 * \return 0 on success or an errno on failure
 */
INTERNAL int sysexit(proc_t *proc);

/** handle end of `access`
 *
 * \param proc Process that made this syscall
 * \return 0 on success or an errno on failure
 */
INTERNAL int sysexit_access(proc_t *proc);

/** handle end of `chdir`
 *
 * \param proc Process that made this syscall
 * \return 0 on success or an errno on failure
 */
INTERNAL int sysexit_chdir(proc_t *proc);

/** handle end of `close`
 *
 * \param proc Process that made this syscall
 * \return 0 on success or an errno on failure
 */
INTERNAL int sysexit_close(proc_t *proc);

/** handle end of `newfstatat`
 *
 * \param proc Process that made this syscall
 * \return 0 on success or an errno on failure
 */
INTERNAL int sysexit_newfstatat(proc_t *proc);

/** handle end of `openat`
 *
 * \param proc Process that made this syscall
 * \return 0 on success or an errno on failure
 */
INTERNAL int sysexit_openat(proc_t *proc);

/** convert a syscall number to its name
 *
 * \param number Syscall to lookup
 * \return The name of the syscall of "<unknown>" if there was no match
 */
INTERNAL const char *syscall_to_str(unsigned long number);
