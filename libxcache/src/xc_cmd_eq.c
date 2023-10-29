#include <stdbool.h>
#include <string.h>
#include <xcache/cmd.h>

bool xc_cmd_eq(const xc_cmd_t a, const xc_cmd_t b) {

  if (a.argc != b.argc)
    return false;

  for (size_t i = 0; i < a.argc; ++i) {
    if (strcmp(a.argv[i], b.argv[i]) != 0)
      return false;
  }

  if (strcmp(a.cwd, b.cwd) != 0)
    return false;

  return true;
}
