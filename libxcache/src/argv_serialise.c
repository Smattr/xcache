#include "argv_serialise.h"
#include "debug.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

char *argv_serialise(size_t argc, char **argv) {

  assert(argc > 0);
  assert(argv != NULL);

  // determine how much memory we need to allocate
  size_t len = 0;
  for (size_t i = 0; i < argc; ++i) {
    assert(argv[i] != NULL);
    len += strlen(argv[i]) + 1;
  }

  // allocate a buffer to serialise into
  char *r = malloc(sizeof(char) * len);
  if (ERROR(r == NULL))
    return NULL;

  // serialise, using Record Separator as a delimiter
  static const char DELIMITER = 31;
  size_t offset = 0;
  for (size_t i = 0; i < argc; ++i) {
    strcpy(&r[offset], argv[i]);
    offset += strlen(argv[i]);
    r[offset] = DELIMITER;
    ++offset;
  }
  assert(offset == len);
  r[offset - 1] = '\0';

  return r;
}
