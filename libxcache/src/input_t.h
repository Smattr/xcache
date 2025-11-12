#pragma once

#include "../../common/compiler.h"
#include "hash_t.h"
#include "list.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>

/// type of a recorded file system read
typedef enum {
  INP_ACCESS,   ///< access()
  INP_READ,     ///< open() with O_RDONLY or O_RDWR
  INP_READLINK, ///< readlink()
  INP_STAT,     ///< stat()
} input_type_t;

/// a file/directory that was read
typedef struct {
  input_type_t tag; ///< discriminator of the union
  char *path;       ///< absolute path to the target of this action
  int err;          ///< any errno that resulted
  union {
    struct {
      int flags; ///< mask of F_OK, R_OK, W_OK, X_OK
    } access;
    struct {
      hash_t hash; ///< hash of the file’s content
    } read;
    struct {
      hash_t hash; ///< hash of the link’s target
    } readlink;
    struct {
      bool is_lstat : 1;    ///< were symlinks not followed?
      mode_t mode;          ///< mode of the file/directory
      uid_t uid;            ///< user ID
      gid_t gid;            ///< group ID
      size_t size;          ///< on-disk size in bytes
      struct timespec mtim; ///< modification time
      struct timespec ctim; ///< creation time
    } stat;
  };
} input_t;

/// create an input for an access() call
///
/// @param input [out] Created input on success
/// @param expected_err Expected error result
/// @param path Absolute path to the target file/directory
/// @param flags Flags to access()
/// @return 0 on success or an errno on failure
INTERNAL int input_new_access(input_t *input, int expected_err,
                              const char *path, int flags);

/// create an input for a read open() call
///
/// @param input [out] Created input on success
/// @param expected_err Expected error result
/// @param path Absolute path to the target file/directory
/// @return 0 on success or an errno on failure
INTERNAL int input_new_read(input_t *input, int expected_err, const char *path);

/// create an input for a readlink() call
///
/// @param input [out] Created input on success
/// @param expected_err Expected error result
/// @param path Absolute path to the link
/// @return 0 on success or an errno on failure
INTERNAL int input_new_readlink(input_t *input, int expected_err,
                                const char *path);

/// create an input for a stat() call
///
/// @param input [out] Created input on success
/// @param expected_err Expected error result
/// @param path Absolute path to the target file/directory
/// @param is_lstat Whether to use `stat` or `lstat`
/// \return 0 on success or an errno on failure
INTERNAL int input_new_stat(input_t *input, int expected_err, const char *path,
                            bool is_lstat);

/// deserialise an input from a file
///
/// @param input [out] Reconstructed input on success
/// @param stream File to read from
/// @return 0 on success or an errno on failure
INTERNAL int input_load(input_t *input, FILE *stream);

/// serialise an input to a file
///
/// @param input Input to write out
/// @param stream File to write to
/// @return 0 on success or an errno on failure
INTERNAL int input_save(const input_t input, FILE *stream);

/// is this input still consistent with how it was originally perceived?
///
/// @param input Input to examine
INTERNAL bool input_is_valid(const input_t input);

/// compare two input actions for equality
///
/// @param a First operand to ==
/// @param b Second operand to ==
/// @return True if the input actions are identical
INTERNAL bool input_eq(const input_t a, const input_t b);

/// destroy an input
///
/// @param i Input whose backing memory to deallocate
INTERNAL void input_free(input_t i);

/// convert an input to a string representation
///
/// This function heap-allocates. Do not call it on a hot path.
///
/// @param input Input to translate
/// @return A string representation of `NULL` on out-of-memory
INTERNAL char *input_to_str(const input_t input);

/// a collection of inputs
typedef LIST(input_t) inputs_t;
