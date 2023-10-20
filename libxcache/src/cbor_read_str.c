#include "cbor.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int cbor_read_str(FILE *stream, char **value) {

  assert(stream != NULL);
  assert(value != NULL);

  const int tag = getc(stream);
  if (tag < 0)
    return EIO;

  size_t len = 0;
  if (tag >= 0x60 && tag <= 0x77) {
    len = (size_t)tag - 0x60;
  } else if (tag == 0x78) {
    uint8_t l = 0;
    if (fread(&l, sizeof(l), 1, stream) < 1)
      return EIO;
    len = l;
  } else if (tag == 0x79) {
    uint16_t l = 0;
    if (fread(&l, sizeof(l), 1, stream) < 1)
      return EIO;
    len = be16toh(l);
  } else if (tag == 0x7a) {
    uint32_t l = 0;
    if (fread(&l, sizeof(l), 1, stream) < 1)
      return EIO;
    len = be32toh(l);
  } else if (tag == 0x7b) {
    uint64_t l = 0;
    if (fread(&l, sizeof(l), 1, stream) < 1)
      return EIO;
    len = be64toh(l);
  } else {
    return EIO;
  }

  if (SIZE_MAX - 1 < len)
    return EOVERFLOW;
  char *v = calloc(1, len + 1);
  if (v == NULL)
    return ENOMEM;

  if (fread(v, len, 1, stream) < 1) {
    free(v);
    return EIO;
  }

  *value = v;
  return 0;
}
