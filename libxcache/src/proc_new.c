#include "macros.h"
#include "proc.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <xcache/proc.h>

int xc_proc_new(xc_proc_t **proc, int argc, char **argv, const char *cwd) {

  if (UNLIKELY(proc == NULL))
    return EINVAL;

  if (UNLIKELY(argc < 1))
    return EINVAL;

  if (UNLIKELY(argv == NULL))
    return EINVAL;

  for (int i = 0; i < argc; ++i) {
    if (UNLIKELY(argv[i] == NULL))
      return EINVAL;
  }

  if (UNLIKELY(cwd == NULL))
    return EINVAL;

  if (UNLIKELY(cwd[0] != '/'))
    return EINVAL;

  int rc = ENOMEM;

  xc_proc_t *p = calloc(1, sizeof(*p));
  if (UNLIKELY(p == NULL))
    goto done;

  size_t ac = (size_t)argc;
  p->argv = calloc(ac + 1, sizeof(p->argv[0]));
  if (UNLIKELY(p->argv == NULL))
    goto done;
  p->argc = ac;

  for (size_t i = 0; i < ac; ++i) {
    p->argv[i] = strdup(argv[i]);
    if (UNLIKELY(p->argv[i] == NULL))
      goto done;
  }

  p->cwd = strdup(cwd);
  if (UNLIKELY(p->cwd == NULL))
    goto done;

  rc = 0;

done:
  if (LIKELY(rc == 0)) {
    *proc = p;
  } else {
    xc_proc_free(p);
  }

  return rc;
}
