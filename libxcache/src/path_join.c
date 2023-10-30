#include "path.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

char *path_join(const char *root, const char *stem) {

  assert(root != NULL);
  assert(stem != NULL);

  const size_t root_len = strlen(root);
  assert(root_len > 0);

  const size_t stem_len = strlen(stem);
  size_t total = root_len + stem_len + 1;
  const bool needs_separator = root[root_len - 1] != '/';
  if (needs_separator)
    ++total;

  char *result = calloc(1, total);
  if (result == NULL)
    return NULL;

  size_t offset = 0;
  memcpy(&result[offset], root, root_len);
  offset += root_len;
  if (needs_separator) {
    result[offset] = '/';
    ++offset;
  }
  memcpy(&result[offset], stem, stem_len);

  return result;
}
