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

/** read a ≤64-bit unsigned integer
 *
 * Same as `cbor_read_u64` but supports tweaking how tags are interpreted.
 *
 * \param stream File to read from
 * \param value [out] Result on success
 * \param bias Offset to apply when interpreting tags
 * \return 0 on success or an errno on failure
 */
INTERNAL int cbor_read_u64_raw(FILE *stream, uint64_t *value, uint8_t bias);

/** write a string
 *
 * \param stream File to write to
 * \param value Stirng to write
 * \return 0 on success or an errno on failure
 */
INTERNAL int cbor_write_str(FILE *stream, const char *value);

/** write a ≤64-bit unsigned integer
 *
 * \param stream File to write to
 * \param value Value to write
 * \return 0 on success or an errno on failure
 */
INTERNAL int cbor_write_u64(FILE *stream, uint64_t value);

/** write a ≤64-bit unsigned integer
 *
 * Same as `cbor_write_u64` but supports tweaking how tags are interpreted.
 *
 * \param stream File to write to
 * \param value Value to write
 * \param bias Offset to apply when writing tags
 * \return 0 on success or an errno on failure
 */
INTERNAL int cbor_write_u64_raw(FILE *stream, uint64_t value, uint8_t bias);
