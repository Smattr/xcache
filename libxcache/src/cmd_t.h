#pragma once

#include "../../common/compiler.h"
#include <stdio.h>
#include <xcache/cmd.h>

/** deserialise a command from a file
 *
 * \param cmd [out] Reconstructed command on success
 * \param stream File to read from
 * \return 0 on success or an errno on failure
 */
INTERNAL int cmd_read(xc_cmd_t *cmd, FILE *stream);

/** serialise a command to a file
 *
 * \param cmd Command to write out
 * \param stream File to write to
 * \return 0 on success or an errno on failure
 */
INTERNAL int cmd_write(const xc_cmd_t cmd, FILE *stream);
