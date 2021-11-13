#include "macros.h"
#include "proc.h"
#include "trace.h"
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <xcache/proc.h>
#include <xcache/trace.h>

static int child(const xc_proc_t *proc) {

  // TODO: install seccomp filter

  return xc_proc_exec(proc);
}

int xc_trace_record(xc_trace_t **trace, const xc_proc_t *proc) {

  if (UNLIKELY(trace == NULL))
    return EINVAL;

  if (UNLIKELY(proc == NULL))
    return EINVAL;

  if (UNLIKELY(proc->argc == 0))
    return EINVAL;

  if (UNLIKELY(proc->argv == NULL))
    return EINVAL;

  for (size_t i = 0; i < proc->argc; ++i) {
    if (UNLIKELY(proc->argv[i] == NULL))
      return EINVAL;
  }

  int rc = -1;

  xc_trace_t *t = calloc(1, sizeof(*t));
  if (UNLIKELY(t == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  pid_t pid = fork();
  if (UNLIKELY(pid == -1)) {
    rc = errno;
    goto done;
  }

  // are we the child?
  if (pid == 0) {
    int r = child(proc);
    exit(r);
  }

  // TODO: monitor the child

  // wait for the child to finish
  int status;
  if (UNLIKELY(waitpid(pid, &status, 0) == -1)) {
    rc = errno;
    goto done;
  }

  // decode its exit status
  if (WIFEXITED(status)) {
    t->exit_status = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    t->exit_status = 128 + WTERMSIG(status);
  } else {
    t->exit_status = -1;
  }

  rc = 0;
  *trace = t;

done:
  if (UNLIKELY(rc != 0))
    free(t);

  return rc;
}
