#include "cbor.h"
#include "hash_t.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int hash_load(hash_t *hash, FILE *stream) {

  assert(hash != NULL);
  assert(stream != NULL);

  *hash = (hash_t){0};
  int rc = 0;

  uint64_t data = 0;
  if ((rc = cbor_read_u64(stream, &data)))
    goto done;

  hash->data = data;

done:
  return rc;
}
