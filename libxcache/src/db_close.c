#include "db.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <xcache/db.h>

void xc_db_close(xc_db_t *db) {

  if (db == NULL)
    return;

  (void)sqlite3_close(db->db);
  db->db = NULL;

  free(db->root);
  db->root = NULL;

  free(db);
}
