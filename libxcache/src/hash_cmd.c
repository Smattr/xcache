#include "cmd_t.h"
#include "debug.h"
#include "hash_t.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <xcache/cmd.h>

int hash_cmd(const xc_cmd_t cmd, hash_t *hash) {

  assert(hash != NULL);

  *hash = (hash_t){0};
  char *buffer = NULL;
  size_t buffer_size = 0;
  FILE *stream = NULL;
  int rc = 0;

  // create an in-memory stream
  stream = open_memstream(&buffer, &buffer_size);
  if (ERROR(stream == NULL)) {
    rc = errno;
    goto done;
  }

  // serialise the command into this buffer
  if (ERROR((rc = cmd_write(cmd, stream))))
    goto done;

  // finalise the stream
  if (ERROR(fclose(stream) < 0)) {
    rc = errno;
    goto done;
  }
  stream = NULL;

  // hash the serialised representation
  *hash = hash_data(buffer, buffer_size);

done:
  if (stream != NULL)
    (void)fclose(stream);
  free(buffer);

  return rc;
}
