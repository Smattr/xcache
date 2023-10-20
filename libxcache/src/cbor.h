/// \file
/// \brief serialisation and deserialisation
///
/// Some data structures need to be written to and then later read back from
/// disk. The encoding for this is arbitrary, but we pick CBOR.

#pragma once

#include "../../common/compiler.h"
#include <stdint.h>
#include <stdio.h>

/** read a string
 *
 * \param stream File to read from
 * \param value [out] Resulting string on success
 * \return 0 on success or an errno on failure
 */
INTERNAL int cbor_read_str(FILE *stream, char **value);

/** read a ≤64-bit unsigned integer
 *
 * \param stream File to read from
 * \param value [out] Result on success
 * \return 0 on success or an errno on failure
 */
INTERNAL int cbor_read_u64(FILE *stream, uint64_t *value);
