/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

// This header does not provide much functionality but is meant to demonstrate how,
// in a scratch subdirectory, one can create new programs that are implemented
// in multiple files and headers.

#ifndef SCRATCH_SUBDIR_ADDITIONAL_HEADER_H
#define SCRATCH_SUBDIR_ADDITIONAL_HEADER_H

#include <string>

namespace ns3
{

/**
 * Get a message from the subdir.
 *
 * @return The message from the subdir
 */
std::string ScratchSubdirGetMessage();

} // namespace ns3

#endif // SCRATCH_SUBDIR_ADDITIONAL_HEADER_H
