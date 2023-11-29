#pragma once

#include "../../common/compiler.h"

/** find absolute path to the program we are running within
 *
 * \param exe [out] Path to program that has loaded us
 * \return 0 on success or an errno on failure
 */
INTERNAL int find_exe(char **exe);

/** find absolute path to the containing library
 *
 * \param me [out] Path to ourselves on success
 * \return 0 on success or an errno on failure
 */
INTERNAL int find_lib(char **me);

/** find absolute path to libxcache-spy
 *
 * \param spy [out] Path to libxcache-spy.so on success
 * \return 0 on success or an errno on failure
 */
INTERNAL int find_spy(char **spy);
