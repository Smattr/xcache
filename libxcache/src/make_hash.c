#include "make_hash.h"
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

/// MurmurHash by Austin Appleby
///
/// more information on this at https://github.com/aappleby/smhasher/
static uint64_t MurmurHash64A(const void *key, size_t len) {

  static const uint64_t seed = 0;

  static const uint64_t m = UINT64_C(0xc6a4a7935bd1e995);
  static const unsigned r = 47;

  uint64_t h = seed ^ (len * m);

  const unsigned char *data = key;
  const unsigned char *end = data + len / sizeof(uint64_t) * sizeof(uint64_t);

  while (data != end) {

    uint64_t k;
    memcpy(&k, data, sizeof(k));
    data += sizeof(k);

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  const unsigned char *data2 = data;

  switch (len & 7) {
  case 7:
    h ^= (uint64_t)data2[6] << 48; // fall through
  case 6:
    h ^= (uint64_t)data2[5] << 40; // fall through
  case 5:
    h ^= (uint64_t)data2[4] << 32; // fall through
  case 4:
    h ^= (uint64_t)data2[3] << 24; // fall through
  case 3:
    h ^= (uint64_t)data2[2] << 16; // fall through
  case 2:
    h ^= (uint64_t)data2[1] << 8; // fall through
  case 1:
    h ^= (uint64_t)data2[0];
    h *= m;
  }

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}

xc_hash_t make_hash(const void *data, size_t size) {
  return MurmurHash64A(data, size);
}
