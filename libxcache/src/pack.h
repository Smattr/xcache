#pragma once

#include "macros.h"
#include <stdio.h>
#include <xcache/proc.h>
#include <xcache/trace.h>

/// serialise a process
///
/// \param f File to write serialised representation to
/// \param proc Process to serialise
/// \return 0 on success or an errno on failure
INTERNAL int pack_proc(FILE *f, const xc_proc_t *proc);

/// serialise a trace
///
/// \param f File to write serialised representation to
/// \param trace Trace to serialise
/// \return 0 on success or an errno on failure
INTERNAL int pack_trace(FILE *f, const xc_trace_t *trace);

/// deserialise a trace
///
/// \param base Buffer from which to parse a trace
/// \param size Number of bytes in `buffer`
/// \param trace [out] Deserialised trace on success
/// \return 0 on success or an errno on failure
INTERNAL int unpack_trace(const void *base, size_t size, xc_trace_t **trace);
