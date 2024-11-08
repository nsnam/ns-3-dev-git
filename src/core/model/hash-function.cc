/*
 * Copyright (c) 2012 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "hash-function.h"

#include "log.h"

/**
 * @file
 * @ingroup hash
 * @brief ns3::Hash::Implementation::GetHash64 default implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("HashFunction");

namespace Hash
{

uint64_t
Implementation::GetHash64(const char* buffer, const std::size_t size)
{
    NS_LOG_WARN("64-bit hash requested, only 32-bit implementation available");
    return GetHash32(buffer, size);
}

} // namespace Hash

} // namespace ns3
