#pragma once

#include "macros.h"
#include <stdio.h>
#include <xcache/proc.h>

/// serialise a process
///
/// \param f File to write serialised representation to
/// \param proc Process to serialise
/// \return 0 on success or an errno on failure
INTERNAL int pack_proc(FILE *f, const xc_proc_t *proc);
