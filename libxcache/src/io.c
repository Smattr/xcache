#include "io.h"
#include "input.h"
#include "output.h"

void io_free(io_t io) {
  switch (io.tag) {
  case IO_INPUT:
    input_free(io.input);
    break;
  case IO_OUTPUT:
    output_free(io.output);
    break;
  }
}
