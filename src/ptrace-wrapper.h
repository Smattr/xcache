/* A thin wrapper around ptrace to obviate some of the awkwardness of the API.
 */

#ifndef _XCACHE_PTRACE_WRAPPER_H_
#define _XCACHE_PTRACE_WRAPPER_H_

#include <sys/types.h>

/* Trace the calling process. */
long pt_traceme(void);

/* Trap all fork (and fork-like) events originating from the given process. */
long pt_tracechildren(pid_t pid);

/* Continue execution of the (blocked) process until the next syscall. */
long pt_runtosyscall(pid_t pid);

/* Return the value of the given register in the process's user context. */
long pt_peekreg(pid_t pid, off_t reg);

/* Return the value of the string pointed to by the given register in the
 * process's user context. Returns NULL on failure.
 */
char *pt_peekstring(pid_t pid, off_t reg);

/* Continue execution of a blocked process. */
long pt_continue(pid_t pid);

/* Pass an event, provided as a wait-/waitpid-returned status, to a traced
 * (blocked) process.
 */
void pt_passthrough(pid_t pid, int event);

/* Stop tracing the given (unblocked) process. */
void pt_detach(pid_t pid);

#endif
