#pragma once

#include "../../common/compiler.h"
#include "input_t.h"
#include "output_t.h"
#include <stddef.h>
#include <stdio.h>
#include <xcache/trace.h>

struct xc_trace {
  int root; ///< handle to open trace-containing directory

  input_t *inputs; ///< inputs that were read by the tracee
  size_t n_inputs; ///< number of items in `inputs`

  output_t *outputs; ///< outputs that were written by the tracee
  size_t n_outputs;  ///< number of items in `outputs`
};

/** deserialise a trace from a file
 *
 * \param trace [out] Reconstructed trace on success
 * \param stream File to read from
 * \return 0 on success or an errno on failure
 */
INTERNAL int trace_read(xc_trace_t *trace, FILE *stream);
