#include "debug.h"
#include "pack.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcache/hash.h>
#include <xcache/proc.h>

int xc_hash_proc(xc_hash_t *hash, const xc_proc_t *proc) {

  if (ERROR(hash == NULL))
    return EINVAL;

  if (ERROR(proc == NULL))
    return EINVAL;

  char *buffer_base = NULL;
  FILE *buffer = NULL;
  int rc = -1;

  // serialise the process to get a stream of bytes
  size_t buffer_length = 0;
  buffer = open_memstream(&buffer_base, &buffer_length);
  if (ERROR(buffer == NULL)) {
    rc = errno;
    goto done;
  }
  rc = pack_proc(buffer, proc);
  if (ERROR(rc != 0))
    goto done;
  {
    int r = fclose(buffer);
    buffer = NULL;
    if (r == EOF) {
      rc = errno;
      goto done;
    }
  }

  // derive a hash for this steam
  xc_hash_t h = 0;
  rc = xc_hash_data(&h, buffer_base, buffer_length);
  if (ERROR(rc != 0))
    goto done;

  // success
  *hash = h;
  rc = 0;

done:
  if (buffer != NULL)
    (void)fclose(buffer);
  free(buffer_base);

  return rc;
}
