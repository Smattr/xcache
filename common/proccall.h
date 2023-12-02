/// \file
/// \brief interface for IPC between tracer and subprocess
///
/// The subprocess (tracee) needs to communicate certain out-of-band information
/// to the tracer. That is, actions it takes that need to be recorded but do not
/// result in syscalls that the tracer can see. An intuitive example is `getenv`
/// calls. To do this, libxcache injects an in-process monitoring component
/// (libxcache-spy) into the subprocess.
///
/// libxcache-spy needs a way of communicating back to the tracer. A pipe, like
/// the ones used to capture stdout and stderr, is the obvious solution. But
/// this results in a race condition. Some actions libxcache-spy wants to tell
/// the tracer about (e.g. “ignore the next few syscalls”) will race with
/// syscalls themselves if communicated via a write through a pipe. To avoid
/// this, communication is done via ioctls on the pipe, forcing it to be
/// synchronous and reliably ordered with respect to other syscalls.

#pragma once

/** file descriptor subprocess uses to message the tracer
 *
 * This can be any arbitrary value, with the exception of STDIN_FILENO (0),
 * STDOUT_FILENO (1), or STDERR_FILENO (2).
 */
enum { XCACHE_FILENO = 3 };

/** identifiers for functions to request from the tracer
 *
 * These are arbitrary numbers passed as the `request` argument to `ioctl`. For
 * convenience, they are chosen to be things that decode to an ASCII string
 * shorthand name.
 */
enum {
  CALL_OFF = 0x66666f, ///< stop recording syscalls until seeing `CALL_ON`
  CALL_ON = 0x6e6f,    ///< start recording syscalls again
};

/** get a string representation of a call number
 *
 * \param callno Call number to convert
 * \return String representation
 */
static inline const char *callno_to_str(long callno) {
  if (callno == CALL_OFF)
    return "\"off\"";
  if (callno == CALL_ON)
    return "\"on\"";
  return "<unknown>";
}
