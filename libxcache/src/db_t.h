#pragma once

#include <xcache/db.h>

/// an open database
struct xc_db {
  int root; ///< file descriptor of database directory
};
