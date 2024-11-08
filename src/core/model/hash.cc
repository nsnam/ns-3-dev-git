/*
 * Copyright (c) 2012 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "hash.h"

#include "log.h"

/**
 * @file
 * @ingroup hash
 * @brief ns3::Hasher implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Hash");

Hasher&
GetStaticHash()
{
    static Hasher g_hasher = Hasher();
    g_hasher.clear();
    return g_hasher;
}

Hasher::Hasher()
{
    m_impl = Create<Hash::Function::Murmur3>();
    NS_ASSERT(m_impl);
}

Hasher::Hasher(Ptr<Hash::Implementation> hp)
    : m_impl(hp)
{
    NS_ASSERT(m_impl);
}

Hasher&
Hasher::clear()
{
    m_impl->clear();
    return *this;
}

} // namespace ns3
