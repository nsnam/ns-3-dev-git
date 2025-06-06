/*
 * Copyright (c) 2007,2008,2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 *          Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                               <amine.ismail@UDcast.com>
 */

#include "cid-factory.h"

#include "ns3/uinteger.h"

#include <cstdint>

namespace ns3
{

CidFactory::CidFactory()
    : m_m(0x5500),
      // this is an arbitrary default
      m_basicIdentifier(1),
      m_primaryIdentifier(m_m + 1),
      m_transportOrSecondaryIdentifier(2 * m_m + 1),
      m_multicastPollingIdentifier(0xff00)
{
}

Cid
CidFactory::AllocateBasic()
{
    NS_ASSERT(m_basicIdentifier < m_m);
    m_basicIdentifier++;
    return Cid(m_basicIdentifier);
}

Cid
CidFactory::AllocatePrimary()
{
    NS_ASSERT(m_primaryIdentifier < 2 * m_m);
    m_primaryIdentifier++;
    return Cid(m_primaryIdentifier);
}

Cid
CidFactory::AllocateTransportOrSecondary()
{
    NS_ASSERT(m_transportOrSecondaryIdentifier < 0xfefe);
    m_transportOrSecondaryIdentifier++;
    return Cid(m_transportOrSecondaryIdentifier);
}

Cid
CidFactory::AllocateMulticast()
{
    NS_ASSERT(m_multicastPollingIdentifier < 0xfffd);
    m_multicastPollingIdentifier++;
    return Cid(m_multicastPollingIdentifier);
}

Cid
CidFactory::Allocate(Cid::Type type)
{
    switch (type)
    {
    case Cid::BROADCAST:
        return Cid::Broadcast();
    case Cid::INITIAL_RANGING:
        return Cid::InitialRanging();
    case Cid::BASIC:
        return AllocateBasic();
    case Cid::PRIMARY:
        return AllocatePrimary();
    case Cid::TRANSPORT:
        return AllocateTransportOrSecondary();
    case Cid::MULTICAST:
        return AllocateMulticast();
    case Cid::PADDING:
        return Cid::Padding();
    default:
        NS_FATAL_ERROR("Cannot be reached");
        return 0; // quiet compiler
    }
}

bool
CidFactory::IsTransport(Cid cid) const
{
    int id = cid.m_identifier;
    return (id - 2 * m_m > 0) && (id <= 0xfefe);
}

bool
CidFactory::IsPrimary(Cid cid) const
{
    int id = cid.m_identifier;
    return (id - m_m > 0) && (id <= 2 * m_m);
}

bool
CidFactory::IsBasic(Cid cid) const
{
    uint16_t id = cid.m_identifier;
    return id >= 1 && id <= m_m;
}

void
CidFactory::FreeCid(Cid cid)
{
    /// @todo We need to update the cid bitmap properly here.
    NS_FATAL_ERROR(
        "TODO: Update the cid bitmap properly here-- please implement and contribute a patch");
}

} // namespace ns3
