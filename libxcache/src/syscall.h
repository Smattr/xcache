#pragma once

#include "../../common/compiler.h"
#include "proc_t.h"

/** handle start of a syscall
 *
 * \param proc Process that made this syscall
 * \return 0 on success or an errno on failure
 */
INTERNAL int sysenter(proc_t *proc);

/** handle end of a syscall
 *
 * \param proc Process that made this syscall
 * \return 0 on success or an errno on failure
 */
INTERNAL int sysexit(proc_t *proc);
