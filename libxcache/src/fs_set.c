#include "fs_set.h"
#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <xcache/hash.h>

static int maybe_expand(fs_set_t *set) {

  assert(set != NULL);

  // do we need to expand the set?
  if (UNLIKELY(set->size == set->capacity)) {
    size_t c = set->capacity == 0 ? 128 : (set->capacity * 2);
    fs_t *base = realloc(set->base, sizeof(set->base[0]) * c);
    if (ERROR(base == NULL))
      return ENOMEM;
    set->base = base;
    set->capacity = c;
  }

  return 0;
}

int fs_set_add_read(fs_set_t *set, const char *path) {

  assert(set != NULL);
  assert(path != NULL);
  assert(path[0] == '/' && "path is not absolute");

  // scan to see if this is already in the set
  for (size_t i = 0; i < set->size; ++i) {
    if (strcmp(set->base[i].path, path) == 0) {
      // we can ignore this because it was either:
      //   1. previously read, in which case this is a duplicate read; or
      //   2. previously written, in which case we do not care about tracking
      //      read-back.
      return 0;
    }
  }

  // now we know we need to create a new entry

  // expand the set if necessary
  {
    int r = maybe_expand(set);
    if (ERROR(r != 0))
      return r;
  }

  assert(set->size < set->capacity);

  // hash this file
  // TODO: deal with directories
  xc_hash_t h = 0;
  int r = xc_hash_file(&h, path);
  bool existed = r != ENOENT;
  bool accessible = r != EACCES && r != EPERM;
  if (existed && accessible && r != 0)
    return r;

  // append the new entry
  size_t index = set->size;
  memset(&set->base[index], 0, sizeof(set->base[index]));
  set->base[index].path = strdup(path);
  if (ERROR(set->base[index].path == NULL))
    return ENOMEM;
  set->base[index].read = true;
  set->base[index].existed = existed;
  set->base[index].accessible = accessible;
  set->base[index].hash = h;
  ++set->size;

  return 0;
}

int fs_set_add_write(fs_set_t *set, const char *path) {

  assert(set != NULL);
  assert(path != NULL);
  assert(path[0] == '/' && "path is not absolute");

  // scan to see if this is already in the set
  for (size_t i = 0; i < set->size; ++i) {
    if (strcmp(set->base[i].path, path) == 0) {
      // mark it as written
      set->base[i].written = true;
      return 0;
    }
  }

  // now we know we need to create a new entry

  // expand the set if necessary
  {
    int r = maybe_expand(set);
    if (ERROR(r != 0))
      return r;
  }

  assert(set->size < set->capacity);

  // TODO: deal with directories
  // TODO: when to write the content?

  // append the new entry
  size_t index = set->size;
  memset(&set->base[index], 0, sizeof(set->base[index]));
  set->base[index].path = strdup(path);
  if (ERROR(set->base[index].path == NULL))
    return ENOMEM;
  set->base[index].written = true;
  ++set->size;

  return 0;
}

void fs_set_deinit(fs_set_t *set) {

  assert(set != NULL);

  for (size_t i = 0; i < set->size; ++i) {
    free(set->base[i].path);
    set->base[i].path = NULL;
    free(set->base[i].content_path);
    set->base[i].content_path = NULL;
  }

  free(set->base);
  set->base = NULL;
  set->size = 0;
  set->capacity = 0;
}
