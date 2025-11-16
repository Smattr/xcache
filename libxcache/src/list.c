#include "list.h"
#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __has_feature
#if __has_feature(address_sanitizer)
#include <sanitizer/asan_interface.h>
#define ASAN_POISON(addr, size) ASAN_POISON_MEMORY_REGION((addr), (size))
#define ASAN_UNPOISON(addr, size) ASAN_UNPOISON_MEMORY_REGION((addr), (size))
#endif
#endif

#ifndef ASAN_POISON
#define ASAN_POISON(addr, size)                                                \
  do {                                                                         \
    (void)(addr);                                                              \
    (void)(size);                                                              \
  } while (0)
#endif
#ifndef ASAN_UNPOISON
#define ASAN_UNPOISON(addr, size)                                              \
  do {                                                                         \
    (void)(addr);                                                              \
    (void)(size);                                                              \
  } while (0)
#endif

int list_push_back_(list_impl_t_ *l, const void *item, size_t stride) {
  assert(l != NULL);
  assert(item != NULL);

  int rc = 0;

  // integer overflow?
  if (ERROR(SIZE_MAX - 1 < l->size)) {
    rc = EOVERFLOW;
    goto done;
  }

  if (ERROR((rc = list_reserve_(l, l->size + 1, stride))))
    goto done;

  assert(l->capacity > l->size);

  void *const dst = (void *)((uintptr_t)l->base + l->size * stride);
  ASAN_UNPOISON(dst, stride);
  if (stride > 0)
    memcpy(dst, item, stride);
  ++l->size;

done:
  return rc;
}

void list_pop_(list_impl_t_ *l, size_t index, void *ret, size_t stride) {
  assert(l != NULL);
  assert(ret != NULL || stride == 0);
  assert(index < l->size && "out of bounds access");

  // pop the requested item
  if (stride > 0) {
    const void *const src = (char *)l->base + index * stride;
    memcpy(ret, src, stride);
  }

  // shrink the list
  if (stride > 0) {
    void *const dst = (char *)l->base + index * stride;
    const void *const src = (char *)l->base + (index + 1) * stride;
    memmove(dst, src, (l->size - index - 1) * stride);
  }
  --l->size;

  // poison the (now empty) uppermost slot
  ASAN_POISON((char *)l->base + l->size * stride, stride);
}

void *list_at_(list_impl_t_ *l, size_t index, size_t stride) {
  return (void *)list_at_const_(l, index, stride);
}

const void *list_at_const_(const list_impl_t_ *l, size_t index, size_t stride) {
  assert(index < l->size);
  return (const void *)((uintptr_t)l->base + index * stride);
}

int list_reserve_(list_impl_t_ *l, size_t request, size_t stride) {
  assert(l != NULL);

  // integer overflow?
  if (ERROR(request != 0 && SIZE_MAX / request < stride))
    return EOVERFLOW;

  // if we are already satisfying the request, nothing to do
  if (l->capacity >= request)
    return 0;
  assert(request > 0);

  // opportunistically try doubling if the caller requested less than that
  if (SIZE_MAX / 2 > l->capacity && request / 2 < l->capacity) {
    assert(l->capacity != 0);
    const size_t try = l->capacity * 2;
    void *const base = realloc(l->base, stride * try);
    if (stride == 0 || base != NULL) { // success
      void *const start = (char *)base + l->capacity * stride;
      const size_t len = (try - l->capacity) * stride;
      ASAN_POISON(start, len);

      l->base = base;
      l->capacity = try;
      return 0;
    }
  }

  void *const base = realloc(l->base, stride * request);
  if (ERROR(stride > 0 && base == NULL))
    return ENOMEM;

  void *const start = (char *)base + l->capacity * stride;
  const size_t len = (request - l->capacity) * stride;
  ASAN_POISON(start, len);

  l->base = base;
  l->capacity = request;
  return 0;
}

void list_free_(list_impl_t_ *l) {
  assert(l != NULL);
  free(l->base);
  *l = (list_impl_t_){0};
}
