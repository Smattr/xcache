#include "cbor.h"
#include "cmd_t.h"
#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcache/cmd.h>

int cmd_load(xc_cmd_t *cmd, FILE *stream) {

  assert(cmd != NULL);
  assert(stream != NULL);

  *cmd = (xc_cmd_t){0};
  xc_cmd_t c = {0};
  int rc = 0;

  {
    uint64_t argc = 0;
    if (ERROR((rc = cbor_read_u64_raw(stream, &argc, 0x80))))
      goto done;
    if (ERROR(argc > SIZE_MAX))
      return EOVERFLOW;

    c.argv = calloc(argc + 1, sizeof(c.argv[0]));
    if (ERROR(c.argv == NULL)) {
      rc = ENOMEM;
      goto done;
    }
    c.argc = (size_t)argc;
  }

  for (size_t i = 0; i < c.argc; ++i) {
    if (ERROR((rc = cbor_read_str(stream, &c.argv[i]))))
      goto done;
  }

  if (ERROR((rc = cbor_read_str(stream, &c.cwd))))
    goto done;

  *cmd = c;
  c = (xc_cmd_t){0};

done:
  xc_cmd_free(c);

  return rc;
}
