#include "cbor.h"
#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int cbor_read_str(FILE *stream, char **value) {

  assert(stream != NULL);
  assert(value != NULL);

  char *v = NULL;
  int rc = 0;

  uint64_t len = 0;
  if (ERROR((rc = cbor_read_u64_raw(stream, &len, 0x60))))
    goto done;

  if (ERROR(SIZE_MAX - 1 < len)) {
    rc = EOVERFLOW;
    goto done;
  }
  size_t len_1 = (size_t)(len + 1);
  v = calloc(1, len_1);
  if (ERROR(v == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  if (ERROR(fread(v, len_1 - 1, 1, stream) < 1)) {
    rc = EIO;
    goto done;
  }

  *value = v;
  v = NULL;

done:
  free(v);

  return rc;
}
