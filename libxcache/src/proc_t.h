#pragma once

#include "../../common/compiler.h"

/// a subprocess being traced
typedef struct {
  int outfd[2]; ///< pipe for communicating stdout content
  int errfd[2]; ///< pipe for communicating stderr content
} proc_t;

/** create a new process
 *
 * \param proce [out] Created process on success
 * \return 0 on success or an errno on failure
 */
INTERNAL int proc_new(proc_t *proc);

/** destroy a process
 *
 * \param proc Process to destroy
 */
INTERNAL void proc_free(proc_t proc);
