#pragma once

#include "fs_set.h"
#include "macros.h"
#include <xcache/db.h>
#include <xcache/trace.h>

struct xc_trace {

  /// files accessed
  fs_set_t io;

  /// status the process returned on exit
  int exit_status;
};

/// localise a trace, making its content paths relative to a database root
///
/// A pre-condition is that the content paths of all accessed files are already
/// known relative to the database root.
///
/// \param db Database to make content paths relative to
/// \param local [out] Localised trace on success
/// \param global Trace to localise
/// \return 0 on success or an errno on failure
INTERNAL int trace_localise(const xc_db_t *db, xc_trace_t **local,
                            const xc_trace_t *global);

/// clean up and deallocate the members of a trace
INTERNAL void trace_deinit(xc_trace_t *trace);
