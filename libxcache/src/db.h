#pragma once

#include <sqlite3.h>
#include <xcache/db.h>

struct xc_db {

  /// path to directory hosting the database’s artifacts
  char *root;

  /// handle to the underlying database
  sqlite3 *db;

  /// was the database opened read-only?
  unsigned read_only : 1;
};
