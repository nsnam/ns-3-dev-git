/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "uhr-capabilities.h"

namespace ns3
{

uint16_t
UhrMacCapabilities::GetSize() const
{
    // TODO: size is still TBD in 802.11bn/D0.2
    return 1;
}

void
UhrMacCapabilities::Serialize(Buffer::Iterator& start) const
{
    // TODO: assume DSO supported field is encoded on B3
    uint8_t val = dpsSupport | (dpsAssistingSupport << 1) | (mlPowerManagement << 2) |
                  (dsoSupported << 3) | (npcaSupported << 4) | (bsrEnhancementSupport << 5);

    // TODO: implement other fields once 802.11bn D0.1 is available
    start.WriteU8(val);
}

uint16_t
UhrMacCapabilities::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    const auto val = i.ReadU8();
    dpsSupport = val & 0x01;
    dpsAssistingSupport = (val >> 1) & 0x01;
    mlPowerManagement = (val >> 2) & 0x01;
    // TODO: assume DSO supported field is encoded on B3
    dsoSupported = (val >> 3) & 0x01;
    npcaSupported = (val >> 4) & 0x01;
    bsrEnhancementSupport = (val >> 5) & 0x01;
    // TODO: implement other fields once 802.11bn D0.1 is available
    return 1;
}

uint16_t
UhrPhyCapabilities::GetSize() const
{
    // TODO: size is still TBD in 802.11bn/D0.2
    return 0;
}

void
UhrPhyCapabilities::Serialize(Buffer::Iterator& /*start*/) const
{
    // TODO: implement once 802.11bn D0.1 is available
}

uint16_t
UhrPhyCapabilities::Deserialize(Buffer::Iterator /*start*/)
{
    // TODO: implement once 802.11bn D0.1 is available
    return 0;
}

WifiInformationElementId
UhrCapabilities::ElementId() const
{
    return IE_EXTENSION;
}

WifiInformationElementId
UhrCapabilities::ElementIdExt() const
{
    return IE_EXT_UHR_CAPABILITIES;
}

void
UhrCapabilities::Print(std::ostream& os) const
{
    os << "UHR Capabilities="; // TODO
}

uint16_t
UhrCapabilities::GetInformationFieldSize() const
{
    uint16_t size = 1 + // ElementIdExt
                    m_macCapabilities.GetSize() + m_phyCapabilities.GetSize();
    return size;
}

void
UhrCapabilities::SerializeInformationField(Buffer::Iterator start) const
{
    m_macCapabilities.Serialize(start);
    m_phyCapabilities.Serialize(start);
}

uint16_t
UhrCapabilities::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    uint16_t count = 0;
    Buffer::Iterator i = start;

    uint16_t nBytes = m_macCapabilities.Deserialize(i);
    i.Next(nBytes);
    count += nBytes;

    nBytes = m_phyCapabilities.Deserialize(i);
    i.Next(nBytes);
    count += nBytes;

    return count;
}

} // namespace ns3
