#include "hash_t.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <xcache/cmd.h>

int hash_cmd(const xc_cmd_t cmd, hash_t *hash) {

  assert(hash != NULL);

  *hash = (hash_t){0};
  unsigned char *buffer = NULL;
  int rc = 0;

  // how many bytes do we need to concatenate everything with separating `'\0's?
  size_t len = 0;
  for (size_t i = 0; i < cmd.argc; ++i)
    len += strlen(cmd.argv[i]) + 1;
  len += strlen(cmd.cwd) + 1;

  buffer = calloc(1, len);
  if (buffer == NULL) {
    rc = ENOMEM;
    goto done;
  }

  // serialise everything into the buffer
  {
    size_t offset = 0;
    for (size_t i = 0; i < cmd.argc; ++i) {
      assert(offset < len && "miscalculated length");
      const size_t size = strlen(cmd.argv[i]);
      memcpy(&buffer[offset], cmd.argv[i], size + 1);
      offset += size + 1;
    }
    assert(offset < len && "miscalculated length");
    const size_t size = strlen(cmd.cwd);
    memcpy(&buffer[offset], cmd.cwd, size + 1);
  }

  // compute the hash of this
  *hash = hash_data(buffer, len);

done:
  free(buffer);

  return rc;
}
