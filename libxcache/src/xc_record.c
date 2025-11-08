#include "db_t.h"
#include "debug.h"
#include "event.h"
#include "inferior_t.h"
#include "list.h"
#include "proc_t.h"
#include "syscall.h"
#include "tee_t.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
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
} state_t;

static void *monitor(void *state) {

  assert(state != NULL);

  state_t *st = state;
  inferior_t *inf = &st->inf;
  int rc = 0;

  // start our initial process
  if (ERROR((rc = inferior_start(inf, st->cmd))))
    goto done;

  while (true) {
    int status;
    DEBUG("waiting on child…");
    pid_t tid = waitpid(-1, &status, __WALL | __WNOTHREAD);
    if (ERROR(tid < 0)) {
      if (errno == ECHILD) {
        // all our children are done
        break;
      }
      rc = errno;
      goto done;
    }
    DEBUG("saw an event from TID %ld", (long)tid);
    assert((WIFEXITED(status) || WIFSIGNALED(status) || WIFSTOPPED(status) ||
            WIFCONTINUED(status)) &&
           "unknown waitpid status");
    assert(!WIFCONTINUED(status) &&
           "waitpid indicated SIGCONT when we did not request it");

    // we should not have received any of the events we did not ask for
    assert(!is_exit(status));
    assert(!is_vfork_done(status));

    // locate which process and thread we are dealing with
    proc_t *proc = NULL;
    thread_t *thread = NULL;
    for (size_t i = 0; i < LIST_SIZE(&inf->procs); ++i) {
      proc_t *const p = LIST_AT(&inf->procs, i);
      for (size_t j = 0; j < p->n_threads; ++j) {
        if (p->threads[j].id == tid) {
          proc = p;
          thread = &proc->threads[j];
          break;
        }
      }
      if (thread != NULL)
        break;
    }
    if (ERROR(thread == NULL)) {
      DEBUG("TID %ld is not a child we are tracking", (long)tid);
      inf->fate = inf->fate ? inf->fate : ECHILD;
      continue;
    }

    // did the child exit?
    if (WIFEXITED(status)) {
      DEBUG("TID %ld exited with %d", (long)tid, WEXITSTATUS(status));
      // FIXME: we should be noting the exit of a _thread_ here, not a process
      proc_exit(proc, WEXITSTATUS(status));
      if (ERROR(WEXITSTATUS(status) != EXIT_SUCCESS))
        inf->fate = inf->fate ? inf->fate : ECHILD;
      continue;
    }

    // was the child killed by a signal?
    if (ERROR(WIFSIGNALED(status))) {
      // FIXME: as above, we should be dealing with a _thread_ here
      DEBUG("TID %ld died with signal %d", (long)tid, WTERMSIG(status));
      proc_exit(proc, 128 + WTERMSIG(status));
      inf->fate = inf->fate ? inf->fate : ECHILD;
      continue;
    }

    // if anything else happens, but we have already failed, release this child
    if (inf->fate) {
      const int signal = is_signal(status) ? WSTOPSIG(status) : 0;
      if (ERROR((rc = thread_detach(*thread, signal))))
        inf->fate = inf->fate ? inf->fate : rc;
      continue;
    }

    if (is_fork(status)) {
      DEBUG("child forked");
      // The target called fork (or a cousin of). Unless I have missed
      // something in the ptrace docs, the only way to also trace forked
      // children is to set `PTRACE_O_FORK` and friends on the root process.
      // Unfortunately the result of this is that we get two events that tell
      // us the same thing: a `SIGTRAP` in the parent on fork (this case) and a
      // `SIGSTOP` in the child before execution (handled below). It is simpler
      // to just ignore the `SIGTRAP` in the parent and start tracking the child
      // when we receive its initial `SIGSTOP`.
      if (ERROR((rc = thread_cont(
                     *thread)))) // FIXME: shouldn't we be `thread_syscall`-ing
                                 // if mode == XC_SYSCALL?
        inf->fate = inf->fate ? inf->fate : rc;
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
        if (ERROR((rc = sysexit(inf, proc, thread)))) {
          inf->fate = inf->fate ? inf->fate : rc;
          (void)thread_detach(*thread, 0);
          continue;
        }
      } else {
        if (ERROR((rc = sysenter(inf, proc, thread)))) {
          inf->fate = inf->fate ? inf->fate : rc;
          (void)thread_detach(*thread, 0);
          continue;
        }
      }

      thread->pending_sysexit = !thread->pending_sysexit;
      continue;
    }

    // we do not care about exec events
    if (is_exec(status)) {
      DEBUG("TID %ld, PTRACE_EVENT_EXEC", (long)tid);
      if (inf->mode == XC_SYSCALL) {
        if (ERROR((rc = thread_syscall(*thread))))
          inf->fate = inf->fate ? inf->fate : rc;
      } else {
        if (ERROR((rc = thread_cont(*thread))))
          inf->fate = inf->fate ? inf->fate : rc;
      }
      continue;
    }

    {
      const int sig = WSTOPSIG(status);
      DEBUG("TID %ld, stopped by signal %d", (long)tid, sig);
      if (ERROR((rc = thread_signal(*thread, sig))))
        inf->fate = inf->fate ? inf->fate : rc;
    }
  }

done:
  if (inf->fate)
    rc = inf->fate;

  return (void *)(intptr_t)rc;
}

int xc_record(xc_db_t *db, const xc_cmd_t cmd, unsigned mode, int *exec_status,
              int *exit_status) {

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

  if (ERROR(exec_status == NULL))
    return EINVAL;
  *exec_status = 0;

  if (ERROR(exit_status == NULL))
    return EINVAL;

  char *trace_root = NULL;
  state_t st = {.cmd = cmd};
  inferior_t *inf = &st.inf;
  int rc = 0;

  // find a usable recording mode
  mode = xc_record_modes(mode);
  if (ERROR(mode == 0)) {
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

    // probe whether the initial `execve` failed
    (void)read(inf->exec_status[0], exec_status, sizeof(*exec_status));

    if (ret != NULL) {
      rc = (int)(intptr_t)ret;
      goto done;
    }
  }

  // coalesce the stdout and stderr threads
  if (ERROR((rc = tee_join(inf->t_out))))
    goto done;
  if (ERROR((rc = tee_join(inf->t_err))))
    goto done;

  // save the result
  if (ERROR((rc = inferior_save(inf, cmd, trace_root))))
    goto done;

  // blank the stdout and stderr saved paths so they are retained
  free(inf->t_out->copy_path);
  inf->t_out->copy_path = NULL;
  free(inf->t_err->copy_path);
  inf->t_err->copy_path = NULL;

done:
  // the monitor should have waited on and cleaned up all tracee threads
  for (size_t i = 0; i < LIST_SIZE(&inf->procs); ++i)
    assert(LIST_AT(&inf->procs, i)->n_threads == 0 &&
           "remaining tracee threads");

  if (rc == 0 || rc == ECHILD) {
    assert(LIST_SIZE(&inf->procs) > 0);
    *exit_status = LIST_AT(&inf->procs, 0)->exit_status;
  }

  inferior_free(inf);
  free(trace_root);

  return rc;
}
