#include "input.h"
#include <stdlib.h>

void input_free(input_t i) {
  free(i.path);

  switch (i.tag) {

  case INP_ACCESS:
  case INP_READ:
  case INP_STAT:
  case INP_SYSCONF:
  case INP_UNLINK_PRE:
    break;

  case INP_READLINK:
    free(i.readlink.target);
    break;

  case INP_GETENV:
    free(i.getenv.key);
    free(i.getenv.value);
    break;
  }
}
