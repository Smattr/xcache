/// @file
/// @brief interface for IPC between tracer and subprocess
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
/// this, communication is done via ioctls on an invalid file descriptor,
/// forcing it to be synchronous and reliably ordered with respect to other
/// syscalls. The ioctls themselves have no effect in the tracee and result in
/// an `EBADF`.

#pragma once

#include <errno.h>

/// file descriptor subprocess uses to message the tracer
///
/// This can be any arbitrary value that will never be a valid open file
/// descriptor or `AT_FDCWD`. linux/fcntl.h claims:
///
///   Reserved kernel ranges [-100], [-10000, -40000].
///
/// and uses the value `-10000 - EBADF` for its `FD_INVALID`. Reportedly some
/// userspace software uses `-EBADF` in `*at` syscalls to avoid paths of unknown
/// providence ever being interpreted as relative. So lets do something similar
/// and pick an `EBADF`-based value outside of kernel reserved space.
enum { XCACHE_FILENO = -100 - EBADF };

/// identifiers for functions to request from the tracer
///
/// These are arbitrary numbers passed as the `request` argument to `ioctl`. For
/// convenience, they are chosen to be things that decode to an ASCII string
/// shorthand name.
enum {
  CALL_HELLO = 0x6f6c6568,   ///< initial startup message from the spy
  CALL_OFF = 0x66666f,       ///< stop recording syscalls until seeing `CALL_ON`
  CALL_ON = 0x6e6f,          ///< start recording syscalls again
  CALL_SYSCONF = 0x666e6f63, ///< tracee called `sysconf`
  CALL_GETENV = 0x766e6567,  ///< tracee called `getenv`
  CALL_SETENV = 0x766e6573,  ///< tracee called `setenv`
};

/// get a string representation of a call number
///
/// @param callno Call number to convert
/// @return String representation
static inline const char *callno_to_str(int callno) {
  if (callno == CALL_HELLO)
    return "\"hello\"";
  if (callno == CALL_OFF)
    return "\"off\"";
  if (callno == CALL_ON)
    return "\"on\"";
  if (callno == CALL_SYSCONF)
    return "\"sysconf\"";
  return "<unknown>";
}
