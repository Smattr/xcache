#pragma once

#include "../../common/compiler.h"
#include <xcache/cmd.h>
#include <xcache/db.h>

/// an open database
struct xc_db {
  char *root; ///< database directory
};

/** get the root directory for traces for the given command
 *
 * \param db Database to lookup
 * \param cmd Command to query for
 * \param root [out] The root directory for traces on success
 * \return 0 on success or an errno on failure
 */
INTERNAL int db_trace_root(const xc_db_t db, const xc_cmd_t cmd, char **root);
