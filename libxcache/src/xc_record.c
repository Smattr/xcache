#include "db_t.h"
#include "debug.h"
#include "event.h"
#include "inferior_t.h"
#include "list.h"
#include "syscall.h"
#include "tee_t.h"
#include "thread_t.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <xcache/cmd.h>
#include <xcache/db.h>
#include <xcache/record.h>
#include <xcache/trace.h>

/// state shared between monitor and main thread
typedef struct {
  inferior_t inf;
  const xc_cmd_t cmd;
  bool preload_prepend; ///< prepend to `$LD_PRELOAD`, as opposed to appending
  int *trace_status;    ///< result of tracing
} state_t;

static void *monitor(void *state) {

  assert(state != NULL);

  state_t *st = state;
  inferior_t *inf = &st->inf;
  int rc = 0;

/// mark tracing as unsuccessful, treating any previous failure as sticky
#define FAIL_TRACE(ret)                                                        \
  do {                                                                         \
    *st->trace_status = *st->trace_status ? *st->trace_status : (ret);         \
  } while (0)

  // start our initial process
  if (ERROR((rc = inferior_start(inf, st->cmd, st->preload_prepend))))
    goto done;

  while (true) {
    int status;
    pid_t tid = waitpid(-1, &status, __WALL | __WNOTHREAD);
    if (ERROR(tid < 0)) {
      if (errno == ECHILD) {
        // all our children are done
        break;
      }
      rc = errno;
      goto done;
    }
    assert((WIFEXITED(status) || WIFSIGNALED(status) || WIFSTOPPED(status) ||
            WIFCONTINUED(status)) &&
           "unknown waitpid status");
    assert(!WIFCONTINUED(status) &&
           "waitpid indicated SIGCONT when we did not request it");

    // we should not have received any of the events we did not ask for
    assert(!is_exit(status));
    assert(!is_vfork_done(status));

    // If we stopped due to `PTRACE_O_TRACEEXEC`, `waitpid` will have given us
    // the thread group leader’s ID. Retrieve the actual thread’s ID.
    if (is_exec(status)) {
      unsigned long msg;
      if (ERROR(ptrace(PTRACE_GETEVENTMSG, tid, NULL, &msg) < 0)) {
        FAIL_TRACE(errno);
        // FIXME: what should we do here?
      } else {
        DEBUG("TID %ld remapped to %lu", (long)tid, msg);
        tid = (pid_t)msg;
      }
    }

    // locate which thread we are dealing with
    thread_t *thread = NULL;
    for (size_t i = 0; i < LIST_SIZE(&inf->threads); ++i) {
      thread_t *const candidate = *LIST_AT(&inf->threads, i);
      if (candidate->id == tid) {
        thread = *LIST_AT(&inf->threads, i);
        break;
      }
    }
    if (ERROR(thread == NULL)) {
      // FIXME: is this case actually possible any more or does this always
      // indicate an xcache bug?
      DEBUG("TID %ld is not a child we are tracking", (long)tid);
      FAIL_TRACE(ESRCH);
      continue;
    }

    // did the child exit?
    if (WIFEXITED(status)) {
      DEBUG("TID %ld exited with %d", (long)tid, WEXITSTATUS(status));
      inferior_thread_exit(inf, thread, WEXITSTATUS(status));
      continue;
    }

    // was the child killed by a signal?
    if (ERROR(WIFSIGNALED(status))) {
      // FIXME: as above, we should be dealing with a _thread_ here
      DEBUG("TID %ld died with signal %d", (long)tid, WTERMSIG(status));
      inferior_thread_exit(inf, thread, 128 + WTERMSIG(status));
      continue;
    }

    if (is_fork(status)) {
      DEBUG("TID %ld forked", (long)tid);

      // learn the TID of the new child
      unsigned long msg;
      if (ERROR(ptrace(PTRACE_GETEVENTMSG, tid, NULL, &msg) < 0)) {
        FAIL_TRACE(errno);
        abort(); // TODO
      }
      const pid_t child = (pid_t)msg;

      {
        const int r = inferior_spawn(inf, thread, child);
        if (ERROR(r != 0))
          FAIL_TRACE(r);
      }

      {
        const int r = inferior_thread_continue(inf, thread, 0);
        if (ERROR(r != 0))
          FAIL_TRACE(r);
      }

      continue;
    }

    if (is_seccomp(status)) {
      assert((inf->mode == XC_EARLY_SECCOMP || inf->mode == XC_LATE_SECCOMP) &&
             "received a seccomp stop when we did not request it");
      rc = ENOTSUP; // TODO
      goto done;
    }

    if (is_syscall(status)) {
      if (thread->pending_sysexit) {
        const int r = sysexit(inf, thread);
        if (ERROR(r != 0))
          FAIL_TRACE(r);
      } else {
        const int r = sysenter(inf, thread);
        if (ERROR(r != 0))
          FAIL_TRACE(r);
      }

      thread->pending_sysexit = !thread->pending_sysexit;
      continue;
    }

    if (is_exec(status)) {
      DEBUG("TID %ld, PTRACE_EVENT_EXEC", (long)tid);

      // the spy will be re-initialised in an exec-ed child
      thread->seen_spy_hello = false;

      const int r = inferior_thread_continue(inf, thread, 0);
      if (ERROR(r != 0))
        FAIL_TRACE(r);
      continue;
    }

    // if this is a new child’s initial `SIGSTOP`, unblock it
    assert(WIFSTOPPED(status));
    if (WSTOPSIG(status) == SIGSTOP && thread->pending_sigstop) {
      DEBUG("TID %ld’s initial SIGSTOP", (long)tid);
      const int r = inferior_thread_continue(inf, thread, 0);
      if (ERROR(r != 0))
        FAIL_TRACE(r);
      continue;
    }

    {
      const int sig = WSTOPSIG(status);
      DEBUG("TID %ld, stopped by signal %d", (long)tid, sig);
      const int r = inferior_thread_continue(inf, thread, sig);
      if (ERROR(r != 0))
        FAIL_TRACE(r);
    }
  }

done:
  return (void *)(intptr_t)rc;
}

int xc_record(xc_db_t *db, const xc_cmd_t cmd, unsigned mode,
              xc_record_t *status) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(cmd.argc == 0))
    return EINVAL;

  if (ERROR(cmd.argv == NULL))
    return EINVAL;

  for (size_t i = 0; i < cmd.argc; ++i) {
    if (ERROR(cmd.argv[i] == NULL))
      return EINVAL;
  }

  if (ERROR((mode & XC_MODE_AUTO) == 0))
    return EINVAL;

  if (ERROR((mode & (XC_PRELOAD_PREPEND | XC_PRELOAD_APPEND)) == 0))
    return EINVAL;

  if (ERROR(status == NULL))
    return EINVAL;

  *status = (xc_record_t){0};
  char *trace_root = NULL;
  state_t st = {.cmd = cmd,
                .preload_prepend = !!(mode & XC_PRELOAD_PREPEND),
                .trace_status = &status->trace_status};
  inferior_t *inf = &st.inf;
  int rc = 0;

  // find a usable recording mode
  mode = xc_record_modes(mode);
  if (ERROR((mode & XC_MODE_AUTO) == 0)) {
    rc = ENOSYS;
    goto done;
  }

  // derive the trace root directory
  if (ERROR((rc = db_trace_root(*db, cmd, &trace_root))))
    goto done;

  // create it if it does not exist
  if (mkdir(trace_root, 0755) < 0) {
    if (ERROR(errno != EEXIST)) {
      rc = errno;
      goto done;
    }
  }

  if (ERROR((rc = inferior_new(inf, mode, trace_root))))
    goto done;

  // We want to wait on the tracee and any subprocesses and/or threads it spawns
  // but not the tee threads we just created nor on any children of our caller.
  // Linux APIs do not seem to offer a way to do this directly. So spawn a
  // separate thread that can wait on all of its children to exclude the
  // unwanted threads.
  {
    pthread_t mon;
    if (ERROR((rc = pthread_create(&mon, NULL, monitor, &st))))
      goto done;

    void *ret = NULL;
    if (ERROR((rc = pthread_join(mon, &ret))))
      goto done;

    if (ret != NULL) {
      rc = (int)(intptr_t)ret;
      goto done;
    }
  }

  // probe whether the initial `execve` failed
  (void)read(inf->exec_status[0], &status->exec_status,
             sizeof(status->exec_status));

  // if `execve` failed, fail tracing
  if (status->exec_status != 0 && status->trace_status == 0)
    status->trace_status = status->exec_status;

  // coalesce the stdout and stderr threads
  if (ERROR((rc = tee_join(inf->t_out))))
    goto done;
  if (ERROR((rc = tee_join(inf->t_err))))
    goto done;

  if (status->trace_status == 0) {
    // save the result
    if (ERROR((rc = inferior_save(inf, cmd, trace_root))))
      goto done;

    // blank the stdout and stderr saved paths so they are retained
    free(inf->t_out->copy_path);
    inf->t_out->copy_path = NULL;
    free(inf->t_err->copy_path);
    inf->t_err->copy_path = NULL;
  }

done:
  // the monitor should have waited on and cleaned up all tracee threads
  assert(LIST_SIZE(&inf->threads) == 0 && "remaining tracee threads");

  if (rc == 0 && status->exec_status == 0)
    status->exit_status = inf->exit_status;

  inferior_free(inf);
  free(trace_root);

  return rc;
}
