#include "set.h"
#include "debug.h"
#include "hash_t.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/// does this set contain the universe?
static bool is_universe(const set_t set) {
  // use `{0, SIZE_MAX, 0}` as a convenient universe representation
  return set.size == SIZE_MAX;
}

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

  // the universe already contains everything, so skip insertion in that case
  if (is_universe(*set))
    return 0;

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
      if (src == NULL)
        continue;
      char **const dst = find_for_insert(d, b, src);
      assert(dst != NULL);
      *dst = src;
    }

    // make the replacement final
    free(set->data);
    set->data = d;
    set->buckets = b;
  }
  assert(set->size < set->buckets);

  // insert the item
  char **const dst = find_for_insert(set->data, set->buckets, item);
  assert(dst != NULL);
  assert(*dst == NULL);
  *dst = strdup(item);
  if (ERROR(*dst == NULL))
    return ENOMEM;
  ++set->size;

  assert(set->size > 0);

  return 0;
}

int set_add_universe(set_t *set) {
  assert(set != NULL);

  set_free(*set);
  *set = (set_t){.size = SIZE_MAX}; // see `is_universe`

  return 0;
}

bool set_contains(const set_t *set, const char *item) {
  assert(set != NULL);
  assert(item != NULL);

  // the universe contains everything
  if (is_universe(*set))
    return true;

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

  if (is_universe(*src))
    return set_add_universe(dst);

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

void set_dump(const set_t *set, FILE *sink, size_t indent) {
  assert(set != NULL);
  assert(sink != NULL);

#define INDENT(levels)                                                         \
  do {                                                                         \
    for (size_t i_ = 0; i_ < (levels); ++i_) {                                 \
      fputs("  ", sink);                                                       \
    }                                                                          \
  } while (0)

  INDENT(indent);
  fputs("(set_t){\n", sink);
  INDENT(indent + 1);
  fputs(".data = {", sink);

  for (size_t i = 0; i < set->buckets; ++i) {
    const char *const slot = set->data[i];
    fputc('\n', sink);
    INDENT(indent + 2);
    if (slot == NULL) {
      fputs("NULL,", sink);
    } else {
      fprintf(sink, "\"%s\",", slot);
    }
  }

  fputc('\n', sink);
  INDENT(indent + 1);
  fputs("},", sink);

  fputc('\n', sink);
  INDENT(indent + 1);
  fprintf(sink, ".size = %zu,", set->size);

  fputc('\n', sink);
  INDENT(indent + 1);
  fprintf(sink, ".buckets = %zu,", set->buckets);

  fputc('\n', sink);
  INDENT(indent);
  fputc('}', sink);

#undef INDENT
}

void set_free(set_t set) {
  for (size_t i = 0; i < set.buckets; ++i)
    free(set.data[i]);
  free(set.data);
}
