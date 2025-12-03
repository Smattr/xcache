#include "set.h"
#include "debug.h"
#include "hash_t.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

static size_t hash(const char *s) {
  assert(s != NULL);
  const hash_t h = hash_str(s);
  return (size_t)h.data;
}

static char **find_for_insert(char **data, size_t buckets, const char *item) {
  assert(data != NULL || buckets == 0);
  assert(item != NULL);

  const size_t h = hash(item);
  for (size_t i = 0; i < buckets; ++i) {
    const size_t index = h % buckets + i;
    if (data[index] == NULL)
      return &data[index];
  }

  return NULL;
}

int set_add(set_t *set, const char *item) {
  assert(set != NULL);
  assert(item != NULL);

  // do we need to expand the backing storage?
  enum { OCCUPANCY_THRESHOLD = 70 /* percent */ };
  if (set->size >= set->buckets * OCCUPANCY_THRESHOLD / 100) {
    const size_t b = set->buckets == 0 ? 1 : set->buckets * 2;
    char **const d = calloc(b, sizeof(d[0]));
    if (ERROR(d == NULL))
      return ENOMEM;

    // migrate everything into the new set
    for (size_t i = 0; i < set->buckets; ++i) {
      char *const src = set->data[i];
      char **const dst = find_for_insert(d, b, src);
      assert(dst != NULL);
      *dst = src;
    }

    // make the replacement final
    free(set->data);
    set->data = d;
    set->buckets = b;
  }

  // insert the item
  char **const dst = find_for_insert(set->data, set->buckets, item);
  assert(dst != NULL);
  assert(*dst == NULL);
  *dst = strdup(item);
  if (ERROR(*dst == NULL))
    return ENOMEM;

  return 0;
}

bool set_contains(const set_t *set, const char *item) {
  assert(set != NULL);
  assert(item != NULL);

  const size_t h = hash(item);
  for (size_t i = 0; i < set->buckets; ++i) {
    const size_t index = h % set->buckets + i;
    const char *const candidate = set->data[index];
    if (candidate == NULL)
      return false;
    if (strcmp(item, candidate) == 0)
      return true;
  }

  return false;
}

int set_copy(set_t *dst, const set_t *src) {
  assert(dst != NULL);
  assert(src != NULL);

  *dst = (set_t){0};
  set_t d = {0};
  int rc = 0;

  d.data = calloc(src->buckets, sizeof(d.data[0]));
  if (ERROR(src->buckets > 0 && d.data == NULL)) {
    rc = ENOMEM;
    goto done;
  }
  d.buckets = src->buckets;

  for (size_t i = 0; i < src->buckets; ++i) {
    if (src->data[i] == NULL)
      continue;
    d.data[i] = strdup(src->data[i]);
    if (ERROR(d.data[i] == NULL)) {
      rc = ENOMEM;
      goto done;
    }
    ++d.size;
  }

  assert(d.size == src->size && "corrupted source set size");

  *dst = d;
  d = (set_t){0};

done:
  set_free(d);

  return rc;
}

void set_free(set_t set) {
  for (size_t i = 0; i < set.buckets; ++i)
    free(set.data[i]);
  free(set.data);
}
