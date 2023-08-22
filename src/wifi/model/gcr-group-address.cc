/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "gcr-group-address.h"

#include "ns3/address-utils.h"

namespace ns3
{

void
GcrGroupAddress::Print(std::ostream& os) const
{
    os << "gcrGroupAddress=" << m_gcrGroupAddress;
}

WifiInformationElementId
GcrGroupAddress::ElementId() const
{
    return IE_GCR_GROUP_ADDRESS;
}

uint16_t
GcrGroupAddress::GetInformationFieldSize() const
{
    return 6; // GCR Group Address field
}

void
GcrGroupAddress::SerializeInformationField(Buffer::Iterator start) const
{
    WriteTo(start, m_gcrGroupAddress);
}

uint16_t
GcrGroupAddress::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    ReadFrom(start, m_gcrGroupAddress);
    return 6;
}

} // namespace ns3
