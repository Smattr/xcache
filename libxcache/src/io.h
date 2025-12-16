/// @file
/// @brief Abstraction for external inputs/outputs

#pragma once

#include "../../common/compiler.h"
#include "input.h"
#include "output.h"

/// type of an external data flow
typedef enum {
  IO_INPUT,  ///< an input
  IO_OUTPUT, ///< an output
} io_type_t;

/// an external data flow
typedef struct {
  io_type_t tag; ///< discriminator for the union
  union {
    input_t input;
    output_t output;
  };
} io_t;

/// the category of dependency that exists between two sequential actions
typedef enum {
  DEP_NONE, ///< independence
  DEP_RAR,  ///< read-after-read, “input dependence”
  DEP_RAW,  ///< read-after-write, “true dependence”
  DEP_WAR,  ///< write-after-read, “anti-dependence”
  DEP_WAW,  ///< write-after-write, “output dependence”
  DEP_CTRL, ///< control flow dependence
} dep_t;

/// derive the dependency between two actions
///
/// This function is intended to be called by an optimisation algorithm, looking
/// to “hoist” or “sink” operations through a sequential “program”. The actions
/// `a` and `b` are assumed to be part of a program wherein they have been
/// ordered to be adjacent `a;b`.
///
/// The return value of this function effectively means:
///   • `DEP_NONE` – You can freely reorder a and b.
///   • `DEP_RAR` – You can de-dupe a and b.
///   • `DEP_RAW` – You can discard b.
///   • `DEP_WAR` – You must retain a and b, and can stop your analysis.
///   • `DEP_WAW` – You can discard a.
///   • `DEP_CTRL` – You must retain a and b, and can stop your analysis.
/// `DEP_CTRL` is basically the catch-all “a and b have a complex dependency”.
///
/// @param a First action to consider
/// @param b Second action to consider
/// @return Dependency between `a` and `b`
INTERNAL dep_t io_cmp(const io_t a, const io_t b);

/// deallocate resources associated with an `io_t`
///
/// @param io Object to free
INTERNAL void io_free(io_t io);
