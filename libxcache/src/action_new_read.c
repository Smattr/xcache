#include "action_t.h"
#include "debug.h"
#include "hash_t.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

action_t *action_new_read(const char *path) {

  assert(path != NULL);

  action_t *a = NULL;
  action_t *ret = NULL;

  a = calloc(1, sizeof(*a));
  if (ERROR(a == NULL))
    goto done;

  a->tag = ACT_READ;

  a->path = strdup(path);
  if (ERROR(a->path == NULL))
    goto done;

  // TODO: handle EISDIR, mmap failure
  a->err = hash_file(path, &a->read.hash);

  ret = a;
  a = NULL;

done:
  action_free(a);

  return ret;
}
