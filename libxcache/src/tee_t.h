#pragma once

#include "../../common/compiler.h"
#include <pthread.h>
#include <stdbool.h>

/// an IO thread with one source and two sinks
typedef struct {
  int pipe[2];         ///< pipe through which data arrives
  int output_fd;       ///< main output sink
  char *copy_path;     ///< absolute path to teed output sink
  int copy_fd;         ///< teed output sink
  pthread_t forwarder; ///< IO thread
  bool initialised;    ///< has `forwarder` been created?
} tee_t;

/// create a new IO thread and start it running
///
/// @param tee [out] Created thread on success
/// @param output_fd Main output sink
/// @param trace_root Directory in which to create the teed sink
/// @return 0 on success or an errno on failure
INTERNAL int tee_new(tee_t **tee, int output_fd, const char *trace_root);

/// abort execution of an IO thread
///
/// This will also delete the teed output file, if it has been created.
///
/// @param tee Thread to cancel
INTERNAL void tee_cancel(tee_t *tee);

/// signal an IO thread to finish and wait for its exit
///
/// This function does its best to cleanup and deallocate resources, even if the
/// thread exits with non-zero status.
///
/// @param tee Thread to signal
/// @return 0 on success or an errno on failure
INTERNAL int tee_join(tee_t *tee);
