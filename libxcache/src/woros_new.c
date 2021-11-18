#include <assert.h>
#include <errno.h>
#include "macros.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "woros.h"

int woros_new(woros_t *woros, size_t size) {

  assert(woros != NULL);
  assert(size > 0);

  memset(woros, 0, sizeof(*woros));

  int rc = 0;

  woros->base = calloc(size, sizeof(woros->base[0]));
  if (UNLIKELY(woros->base == NULL))
    return ENOMEM;
  woros->size = size;

  for (size_t i = 0; i < size; ++i) {
    if (UNLIKELY(pipe(woros->base[i].fds) != 0)) {
      rc = errno;
      goto done;
    }
  }

done:
  if (rc != 0)
    woros_free(woros);

  return rc;
}
