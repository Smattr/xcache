#include "debug.h"
#include "macros.h"
#include "make_hash.h"
#include <errno.h>
#include <stddef.h>
#include <xcache/hash.h>

int xc_hash_data(xc_hash_t *hash, const void *data, size_t size) {

  if (ERROR(hash == NULL))
    return EINVAL;

  if (ERROR(size > 0 && data == NULL))
    return EINVAL;

  *hash = make_hash(data, size);
  return 0;
}
