/* Syscall wrapper for tkill(). Apparently Glibc doesn't provide such. */

#ifndef _XCACHE_TKILL_H_
#define _XCACHE_TKILL_H_

#include <sys/syscall.h>
#include <unistd.h>

/** \brief Send a signal to a thread. For more information, see `man tkill`. */
static int tkill(int tid, int sig) {
    return syscall(__NR_tkill, tid, sig);
}

#endif
