#include "cbor.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int cbor_read_str(FILE *stream, char **value) {

  assert(stream != NULL);
  assert(value != NULL);

  uint64_t len = 0;
  {
    int rc = cbor_read_u64_raw(stream, &len, 0x60);
    if (rc)
      return rc;
  }

  if (SIZE_MAX - 1 < len)
    return EOVERFLOW;
  size_t len_1 = (size_t)(len + 1);
  char *v = calloc(1, len_1);
  if (v == NULL)
    return ENOMEM;

  if (fread(v, len_1 - 1, 1, stream) < 1) {
    free(v);
    return EIO;
  }

  *value = v;
  return 0;
}
