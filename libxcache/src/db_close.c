#include "db.h"
#include <stdlib.h>
#include <xcache/db.h>

void xc_db_close(xc_db_t *db) {

  if (db == NULL)
    return;

  free(db->root);
  db->root = NULL;

  free(db);
}
