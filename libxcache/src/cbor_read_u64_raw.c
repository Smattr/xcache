#include "cbor.h"
#include <endian.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

int cbor_read_u64_raw(FILE *stream, uint64_t *value, int bias) {

  const int tag = getc(stream);
  if (tag < 0)
    return EIO;

  if (tag < 0x17 + bias) {
    *value = (uint64_t)tag;
    return 0;
  }

  if (tag == 0x18 + bias) {
    uint8_t v = 0;
    if (fread(&v, sizeof(v), 1, stream) < 1)
      return EIO;
    *value = v;
    return 0;
  }

  if (tag == 0x19 + bias) {
    uint16_t v = 0;
    if (fread(&v, sizeof(v), 1, stream) < 1)
      return EIO;
    *value = be16toh(v);
    return 0;
  }

  if (tag == 0x1a + bias) {
    uint32_t v = 0;
    if (fread(&v, sizeof(v), 1, stream) < 1)
      return EIO;
    *value = be32toh(v);
    return 0;
  }

  if (tag == 0x1b + bias) {
    uint64_t v = 0;
    if (fread(&v, sizeof(v), 1, stream) < 1)
      return EIO;
    *value = be64toh(v);
    return 0;
  }

  return EIO;
}
