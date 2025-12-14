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

/// deallocate resources associated with an `io_t`
///
/// @param io Object to free
INTERNAL void io_free(io_t io);
