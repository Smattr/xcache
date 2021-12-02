#pragma once

#include "macros.h"
#include <sqlite3.h>
#include <stdio.h>
#include <xcache/db.h>

struct xc_db {

  /// path to directory hosting the database’s artifacts
  char *root;

  /// handle to the underlying database
  sqlite3 *db;

  /// was the database opened read-only?
  unsigned read_only : 1;
};

/// create a file for capturing a trace output into
///
/// \param db Database in which to home the trace output
/// \param fp [out] Write handle to the created file on success
/// \param path [out] Path to the created file on success
/// \return 0 on success or an errno on failure
INTERNAL int db_make_file(xc_db_t *db, FILE **fp, char **path);
