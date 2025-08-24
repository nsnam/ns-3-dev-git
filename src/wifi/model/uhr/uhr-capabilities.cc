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
UhrCapabilities::UhrMacCapabilities::GetSize() const
{
    // TODO: size is still not defined in 802.11bn/D1.0
    return 8;
}

void
UhrCapabilities::UhrMacCapabilities::Serialize(Buffer::Iterator& start) const
{
    uint64_t val =
        dpsSupport | (dpsAssistingSupport << 1) | (dpsApStaticHcmSupport << 2) |
        (mlPowerManagement << 3) | (npcaSupport << 4) | (enhancedBsrSupport << 5) |
        (additionalMappedTidSupport << 6) | (eotspSupport << 7) |
        (static_cast<uint64_t>(dsoSupport) << 8) | (static_cast<uint64_t>(pEdcaSupport) << 9) |
        (static_cast<uint64_t>(dbeSupport) << 10) | (static_cast<uint64_t>(ulLliSupport) << 11) |
        (static_cast<uint64_t>(p2pLliSupport) << 12) | (static_cast<uint64_t>(puoSupport) << 13) |
        (static_cast<uint64_t>(apPuoSupport) << 14) | (static_cast<uint64_t>(duoSupport) << 15) |
        (static_cast<uint64_t>(omCtrlUlMuDataDisableRxSupport) << 16) |
        (static_cast<uint64_t>(aomSupport) << 17) |
        (static_cast<uint64_t>(ifcsLocationSupport) << 18) |
        (static_cast<uint64_t>(uhrTrsSupport) << 19) | (static_cast<uint64_t>(txspgSupport) << 20) |
        (static_cast<uint64_t>(txspgTxopReturnSupport) << 21) |
        (static_cast<uint64_t>(uhrOpModeAndParamsUpdateTimeout) << 22) |
        (static_cast<uint64_t>(paramUpdateAdvNotificationInterval) << 26) |
        (static_cast<uint64_t>(updateIndicationTimIntervals) << 29) |
        (static_cast<uint64_t>(boundedEss) << 34) | (static_cast<uint64_t>(btmAssurance) << 35);

    // TODO: implement other fields once next 802.11bn draft is available
    start.WriteHtolsbU64(val);
}

uint16_t
UhrCapabilities::UhrMacCapabilities::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    const auto val = i.ReadLsbtohU64();
    dpsSupport = val & 0x01;
    dpsAssistingSupport = (val >> 1) & 0x01;
    dpsApStaticHcmSupport = (val >> 2) & 0x01;
    mlPowerManagement = (val >> 3) & 0x01;
    npcaSupport = (val >> 4) & 0x01;
    enhancedBsrSupport = (val >> 5) & 0x01;
    additionalMappedTidSupport = (val >> 6) & 0x01;
    eotspSupport = (val >> 7) & 0x01;
    dsoSupport = (val >> 8) & 0x01;
    pEdcaSupport = (val >> 9) & 0x01;
    dbeSupport = (val >> 10) & 0x01;
    ulLliSupport = (val >> 11) & 0x01;
    p2pLliSupport = (val >> 12) & 0x01;
    puoSupport = (val >> 13) & 0x01;
    apPuoSupport = (val >> 14) & 0x01;
    duoSupport = (val >> 15) & 0x01;
    omCtrlUlMuDataDisableRxSupport = (val >> 16) & 0x01;
    aomSupport = (val >> 17) & 0x01;
    ifcsLocationSupport = (val >> 18) & 0x01;
    uhrTrsSupport = (val >> 19) & 0x01;
    txspgSupport = (val >> 20) & 0x01;
    txspgTxopReturnSupport = (val >> 21) & 0x01;
    uhrOpModeAndParamsUpdateTimeout = (val >> 22) & 0x0f;
    paramUpdateAdvNotificationInterval = (val >> 26) & 0x07;
    updateIndicationTimIntervals = (val >> 29) & 0x1f;
    boundedEss = (val >> 34) & 0x01;
    btmAssurance = (val >> 35) & 0x01;

    // TODO: implement other fields once next 802.11bn draft is available
    return i.GetDistanceFrom(start);
}

uint16_t
UhrCapabilities::UhrPhyCapabilities::GetSize() const
{
    // TODO: size is still not defined in 802.11bn/D1.0
    return 8;
}

void
UhrCapabilities::UhrPhyCapabilities::Serialize(Buffer::Iterator& start) const
{
    // TODO: implement once finalized in a next 802.11bn draft
    start.WriteU64(0);
}

uint16_t
UhrCapabilities::UhrPhyCapabilities::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    i.ReadU64();
    return i.GetDistanceFrom(start);
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
    os << "UHR Capabilities=[]"; // TODO
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
