#include "../../common/compiler.h"
#include "input.h"
#include "io.h"
#include "output.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

static dep_t in_cmp_in(const input_t a, const input_t b) {

  // compare, ignoring error value
  {
    input_t x = a;
    x.err = 0;
    input_t y = b;
    y.err = 0;
    if (input_eq(x, y))
      return DEP_RAR;
  }

  // otherwise two inputs have no dependency
  return DEP_NONE;
}

static dep_t in_cmp_out(const input_t a, const output_t b) {

  if (strcmp(a.path, b.path) == 0) {
    // no need to precisely analyse what type of operation each is because
    // `DEP_WAR` is a “caller, give up” signal
    return DEP_WAR;
  }

  // conservatively consider either operating on a prefix of the other to
  // constitute a dependency
  do {
    const size_t a_len = strlen(a.path);
    if (a_len >= strlen(b.path))
      break;
    if (strncmp(a.path, b.path, a_len) != 0)
      break;
    if (b.path[a_len] != '/')
      break;
    return DEP_CTRL;
  } while (0);
  do {
    const size_t b_len = strlen(b.path);
    if (b_len >= strlen(a.path))
      break;
    if (strncmp(a.path, b.path, b_len) != 0)
      break;
    if (a.path[b_len] != '/')
      break;
    return DEP_CTRL;
  } while (0);

  return DEP_NONE;
}

static dep_t in_cmp(const input_t a, const io_t b) {
  switch (b.tag) {
  case IO_INPUT:
    return in_cmp_in(a, b.input);
  case IO_OUTPUT:
    return in_cmp_out(a, b.output);
  }
  UNREACHABLE();
}

static dep_t out_cmp_in(const output_t a, const input_t b) {

  do {
    if (strcmp(a.path, b.path) != 0)
      break;

    // is the input reading only things the output wrote?
    if (a.tag == OUT_WRITE && b.tag == INP_READ)
      return DEP_RAW;

    if (a.tag == OUT_WRITE && a.write.is_creat_excl && b.tag == INP_ACCESS)
      return DEP_RAW;

    // assume if we created somethiing, we are able to delete it
    if (a.tag == OUT_WRITE && a.write.is_creat_excl && b.tag == INP_UNLINK_PRE)
      return DEP_RAW;

    // we do not yet distinguish the details of this dependency
    return DEP_CTRL;
  } while (0);

  // conservatively consider either operating on a prefix of the other to
  // constitute a dependency
  do {
    const size_t a_len = strlen(a.path);
    if (a_len >= strlen(b.path))
      break;
    if (strncmp(a.path, b.path, a_len) != 0)
      break;
    if (b.path[a_len] != '/')
      break;
    return DEP_CTRL;
  } while (0);
  do {
    const size_t b_len = strlen(b.path);
    if (b_len >= strlen(a.path))
      break;
    if (strncmp(a.path, b.path, b_len) != 0)
      break;
    if (a.path[b_len] != '/')
      break;
    return DEP_CTRL;
  } while (0);

  return DEP_NONE;
}

static dep_t out_cmp_out(const output_t a, const output_t b) {

  if (output_eq(a, b))
    return DEP_WAW;

  if (strcmp(a.path, b.path) == 0) {
    // if something was written then deleted, the delete dominates
    if (a.tag == OUT_WRITE && b.tag == OUT_UNLINK) {
      // if the write itself was a creation, we can drop the deletion too
      if (a.write.is_creat_excl)
        return DEP_UNDO;

      return DEP_WAW;
    }

    // conservatively do not analyse anything else for now
    return DEP_CTRL;
  }

  // if either is modifying any prefix of the other, consider them dependent
  do {
    const size_t a_len = strlen(a.path);
    if (a_len >= strlen(b.path))
      break;
    if (strncmp(a.path, b.path, a_len) != 0)
      break;
    if (b.path[a_len] != '/')
      break;
    return DEP_CTRL;
  } while (0);
  do {
    const size_t b_len = strlen(b.path);
    if (b_len >= strlen(a.path))
      break;
    if (strncmp(a.path, b.path, b_len) != 0)
      break;
    if (a.path[b_len] != '/')
      break;
    return DEP_CTRL;
  } while (0);

  return DEP_NONE;
}

static dep_t out_cmp(const output_t a, const io_t b) {
  switch (b.tag) {
  case IO_INPUT:
    return out_cmp_in(a, b.input);
  case IO_OUTPUT:
    return out_cmp_out(a, b.output);
  }
  UNREACHABLE();
}

dep_t io_cmp(const io_t a, const io_t b) {

  switch (a.tag) {
  case IO_INPUT:
    return in_cmp(a.input, b);
  case IO_OUTPUT:
    return out_cmp(a.output, b);
  }
  UNREACHABLE();
}
