#pragma once

#include "../../common/compiler.h"

/** copy data from one file to another
 *
 * \param dst Destination file descriptor
 * \param src Source file descriptor
 * \return 0 on success or an errno on failure.
 */
INTERNAL int cp(int dst, int src);
