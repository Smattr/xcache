#pragma once

#include "../../common/compiler.h"
#include "hash_t.h"
#include <sys/stat.h>

/// type of a recorded file system action
typedef enum {
  ACT_ACCESS, ///< access()
  ACT_CREATE, ///< open() with O_CREAT or creat()
  ACT_READ,   ///< open() with O_RDONLY or O_RDWR
  ACT_STAT,   ///< stat()
  ACT_WRITE,  ///< open() with O_WRONLY or O_RDWR
} action_type_t;

/// a file system action
typedef struct action {
  action_type_t tag; ///< discriminator of the union
  char *path;        ///< absolute path to the target of this action
  int err;           ///< any errno that resulted
  union {
    struct {
      int flags; ///< mask of F_OK, R_OK, W_OK, X_OK
    } access;
    struct {
      mode_t mode; ///< creation mode of the file/directory
    } create;
    struct {
      hash_t hash; ///< hash of the file’s content
    } read;
    struct {
      mode_t mode;          ///< mode of the file/directory
      uid_t uid;            ///< user ID
      gid_t gid;            ///< group ID
      size_t size;          ///< on-disk size in bytes
      struct timespec atim; ///< access time
      struct timespec mtim; ///< modification time
      struct timespec ctim; ///< creation time
    } stat;
  };

  struct action *previous; /// previous action in a linked-list
} action_t;

/** create an action for an access() call
 *
 * \param path Absolute path to the target file/directory
 * \param err Any errno that resulted
 * \param flags Flags to access()
 * \return A created action or `NULL` on out-of-memory
 */
INTERNAL action_t *action_new_access(const char *path, int err, int flags);

/** create an action for a read open() call
 *
 * \param path Absolute path to the target file/directory
 * \return A created action or `NULL` on out-of-memory
 */
INTERNAL action_t *action_new_read(const char *path);

/** destroy an action
 *
 * This assumes the action has been heap allocated and frees the pointer itself.
 * It does not touch `action->previous`. `action_free(NULL)` is a no-op.
 *
 * \param action Action to free
 */
INTERNAL void action_free(action_t *action);

/** destroy an action list
 *
 * Calls `action_free` on all actions within a linked-list chain.
 *
 * \param action Head of the list to destroy
 */
INTERNAL void action_free_all(action_t *action);