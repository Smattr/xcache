/// \file
///
/// abstraction of fixed size buffers with Write-Once, Read-Once slots
///
/// These buffers are expected to be used across process boundaries. That is,
/// one can be created prior to calling `fork` and then used between the parent
/// and child. One of these parties is expected to exclusively write to buffer,
/// indicating their intent in advance by calling `woros_i_am_writer`. The other
/// is expected to exclusively read the buffer, indicating their intent in
/// advance by calling `woros_i_am_reader`.
///
/// The buffer is FIFO; it is only ever possible to read or write the head slot.
/// Both reads and writes block, waiting on the partnering side. That is,
/// read/write is effectively a synchronisation mechanism between the reader and
/// writer.

#pragma once

#include <stddef.h>

/// a Write-Once, Read-Once (WORO) slot
typedef struct {
  int fds[2]; /// <handles to read and write ends of the underlying pipe
} woro_t;

/// a buffer of Write-Once, Read-Once (WORO) slots
typedef struct {
  woro_t *base;
  size_t size;
} woros_t;

/// create a new WORO buffer
///
/// \param woros [out] Created WORO buffer on success
/// \param size Number of entries the buffer should be created with
/// \return 0 on success or an errno on failure
int woros_new(woros_t *woros, size_t size);

/// indicate an intent to be the writer of this WORO buffer
///
/// This assumes the caller has not previously called `woros_i_am_reader`.
///
/// \param woros Buffer to operate on
void woros_i_am_writer(woros_t *woros);

/// indicate an intent to be the reader of this WORO buffer
///
/// This assumes the caller has not previously called `woros_i_am_writer`.
///
/// \param woros Buffer to operate on
void woros_i_am_reader(woros_t *woros);

int woros_write(woros_t *woros, int data);

int woros_read(woros_t *words, int *data);

/// Destroy a WORO buffer and deallocate its associated resources
///
/// \param woros Buffer to operate on
void woros_free(woros_t *woros);
