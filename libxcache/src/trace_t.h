#pragma once

#include "../../common/compiler.h"
#include "input_t.h"
#include "output_t.h"
#include <stddef.h>
#include <stdio.h>
#include <xcache/cmd.h>
#include <xcache/trace.h>

struct xc_trace {
  int root; ///< handle to open trace-containing directory

  xc_cmd_t cmd; ///< command originating this trace

  inputs_t inputs; ///< inputs that were read by the tracee

  output_t *outputs; ///< outputs that were written by the tracee
  size_t n_outputs;  ///< number of items in `outputs`
};

/// deserialise a trace from a file
///
/// @param trace [out] Reconstructed trace on success
/// @param trace_root Absolute path to directory containing the trace
/// @param trace_file Path to file containing the trace record
/// @return 0 on success or an errno on failure
INTERNAL int trace_load(xc_trace_t *trace, const char *trace_root,
                        const char *trace_file);

/// serialise a trace to a file
///
/// @param trace Trace to write out
/// @param stream File to write to
/// @return 0 on success or an errno on failure
INTERNAL int trace_save(const xc_trace_t trace, FILE *stream);
