/// @file
/// @brief type-safe generic list
///
/// This is partly based on
/// https://danielchasehooper.com/posts/typechecked-generic-c-data-structures/

#pragma once

#include "../../common/compiler.h"
#include <assert.h>
#include <stddef.h>

/// generic list, agnostic to the list item type
typedef struct {
  void *base;      ///< backing memory for list items
  size_t size;     ///< number of items in the list
  size_t capacity; ///< available item slots
} list_impl_t_;

/// type-generic list
#define LIST(type)                                                             \
  struct {                                                                     \
    union {                                                                    \
      type *base;                                                              \
      list_impl_t_ impl;                                                       \
    };                                                                         \
  }

static_assert(
    offsetof(list_impl_t_, base) == 0,
    "LIST(<type>).base and LIST(<type>).impl.base will not alias each other");

/// get the size of a list (access this through `LIST_SIZE`)
static inline size_t list_size_(const list_impl_t_ *l) { return l->size; }

/// get the size of a list
#define LIST_SIZE(list) list_size_(&(list)->impl)

/// append an item to a list
///
/// Access this through `LIST_PUSH_BACK`.
///
/// @param l List to append to
/// @param item Item to append
/// @param stride Byte size of list items
/// @return 0 on success or an errno on failure
INTERNAL int list_push_back_(list_impl_t_ *l, const void *item, size_t stride);

/// append an item to a list
#define LIST_PUSH_BACK(list, item)                                             \
  list_push_back_(&(list)->impl, (__typeof__((list)->base[0])[]){(item)},      \
                  sizeof((list)->base[0]))

/// access an item in a list
///
/// Access this through `LIST_AT`.
///
/// @param l List to operate on
/// @param index Index of item to lookup
/// @param stride Byte size of list items
/// @return Pointer to the requested item
INTERNAL void *list_at_(list_impl_t_ *l, size_t index, size_t stride);

/// access an item in a list
///
/// Access this through `LIST_AT`. This is identical to `list_at_` but on a
/// constant list pointer.
///
/// @param l List to operate on
/// @param index Index of item to lookup
/// @param stride Byte size of list items
/// @return Pointer to the requested item
INTERNAL const void *list_at_const_(const list_impl_t_ *l, size_t index,
                                    size_t stride);

/// access an item in a list
#define LIST_AT(list, index)                                                   \
  ((__typeof__(&(list)->base[0]))_Generic(&(list)->impl,                       \
       list_impl_t_ *: list_at_,                                               \
       const list_impl_t_ *: list_at_const_)(&(list)->impl, (index),           \
                                             sizeof((list)->base[0])))

/// pre-allocate backing storage for items
///
/// Access this through `LIST_RESERVE`.
///
/// @param l List to operate on
/// @param request Number of item slots to make available
/// @param stride Byte size of list items
/// @return 0 on success or an errno on failure
INTERNAL int list_reserve_(list_impl_t_ *l, size_t request, size_t stride);

/// pre-allocate backing storage for items
#define LIST_RESERVE(list, request)                                            \
  list_reserve_(&(list)->impl, (request), sizeof((list)->base[0]))

/// deallocate a list’s contents (access this through `LIST_FREE`)
INTERNAL void list_free_(list_impl_t_ *l);

/// deallocate a list’s contents
#define LIST_FREE(list, dtor)                                                  \
  do {                                                                         \
    __typeof__(list) list_ = (list);                                           \
    void (*dtor_)(__typeof__(list_->base[0])) = (dtor);                        \
    for (size_t i_ = 0; dtor_ != NULL && i_ < LIST_SIZE(list_); ++i_) {        \
      dtor_(*LIST_AT(list_, i_));                                              \
    }                                                                          \
    list_free_(&list_->impl);                                                  \
  } while (0)
