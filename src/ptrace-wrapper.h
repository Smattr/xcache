/* A thin wrapper around ptrace to obviate some of the awkwardness of the API.
 */

#ifndef _XCACHE_PTRACE_WRAPPER_H_
#define _XCACHE_PTRACE_WRAPPER_H_

#include <sys/types.h>

/* Trace the calling process. */
long pt_traceme(void);

/* Setup default options for tracing. */
long pt_setoptions(pid_t pid);

/* Continue execution of the (blocked) process until the next syscall. */
long pt_runtosyscall(pid_t pid);

/* Return the value of the given register in the process's user context. */
long pt_peekreg(pid_t pid, off_t reg);

/* Return the value of the string pointed to by the given register in the
 * process's user context. Returns NULL on failure.
 */
char *pt_peekstring(pid_t pid, off_t reg);

/* Return the path pointed to by the file descriptor in the given register.
 * Returns NULL on failure.
 */
char *pt_peekfd(pid_t pid, const char *cwd, off_t reg) __attribute__((nonnull));

/* Retrieve the event message associated with the last ptrace event. */
unsigned long pt_geteventmsg(pid_t pid);

/* Continue execution of a blocked process. */
long pt_continue(pid_t pid);

/* Pass an event, provided as a wait-/waitpid-returned status, to a traced
 * (blocked) process.
 */
void pt_passthrough(pid_t pid, int event);

/* Stop tracing the given (unblocked) process. */
void pt_detach(pid_t pid);

/* Signal that gets delivered when we see a syscall from the target. See `man
 * ptrace` for more information.
 */
#define SIGSYSCALL (SIGTRAP|0x80)

#endif
