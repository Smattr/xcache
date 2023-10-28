#include <stdlib.h>
#include <xcache/cmd.h>

void xc_cmd_free(xc_cmd_t cmd) {

  free(cmd.cwd);

  for (size_t i = 0; i < cmd.argc; ++i)
    free(cmd.argv[i]);
  free(cmd.argv);
}
