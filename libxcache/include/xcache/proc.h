#pragma once

#ifndef XCACHE_API
#define XCACHE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// a process to be run or traced
typedef struct xc_proc xc_proc_t;

/// create a new process object
///
/// \param proc [out] On successful return, a created process object
/// \param argc Number of command line arguments
/// \param argv Command line arguments
/// \param cwd Current working directory of this process
/// \return 0 on success or an errno on failure
XCACHE_API int xc_proc_new(xc_proc_t **proc, int argc, char **argv,
                           const char *cwd);

/// run the given process unmonitored
///
/// On success, this call does not return. It functions like `execve` and
/// friends.
///
/// \param proc The process to run
/// \return An errno value on error
XCACHE_API int xc_proc_exec(const xc_proc_t *proc);

/// delete a process object and free up associated memory
///
/// Calling `xc_proc_free(NULL)` is a no-op.
///
/// \param proc Process object to free
XCACHE_API void xc_proc_free(xc_proc_t *proc);

#ifdef __cplusplus
}
#endif
