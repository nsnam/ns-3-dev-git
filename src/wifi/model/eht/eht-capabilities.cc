/*
 * Copyright (c) 2021 DERONNE SOFTWARE ENGINEERING
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "eht-capabilities.h"

namespace ns3
{

uint16_t
EhtMacCapabilities::GetSize() const
{
    return 2;
}

void
EhtMacCapabilities::Serialize(Buffer::Iterator& start) const
{
    uint16_t val = epcsPriorityAccessSupported | (ehtOmControlSupport << 1) |
                   (triggeredTxopSharingMode1Support << 2) |
                   (triggeredTxopSharingMode2Support << 3) | (restrictedTwtSupport << 4) |
                   (scsTrafficDescriptionSupport << 5) | (maxMpduLength << 6) |
                   (maxAmpduLengthExponentExtension << 8);
    start.WriteHtolsbU16(val);
}

uint16_t
EhtMacCapabilities::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    uint16_t val = i.ReadLsbtohU16();
    epcsPriorityAccessSupported = val & 0x0001;
    ehtOmControlSupport = (val >> 1) & 0x0001;
    triggeredTxopSharingMode1Support = (val >> 2) & 0x0001;
    triggeredTxopSharingMode2Support = (val >> 3) & 0x0001;
    restrictedTwtSupport = (val >> 4) & 0x0001;
    scsTrafficDescriptionSupport = (val >> 5) & 0x0001;
    maxMpduLength = (val >> 6) & 0x0003;
    maxAmpduLengthExponentExtension = (val >> 8) & 0x0001;
    return 2;
}

EhtCapabilities::EhtCapabilities()
    : m_macCapabilities{}
{
}

WifiInformationElementId
EhtCapabilities::ElementId() const
{
    return IE_EXTENSION;
}

WifiInformationElementId
EhtCapabilities::ElementIdExt() const
{
    return IE_EXT_EHT_CAPABILITIES;
}

uint16_t
EhtCapabilities::GetInformationFieldSize() const
{
    uint16_t size = 1 + // ElementIdExt
                    m_macCapabilities.GetSize();
    // FIXME: currently only EHT MAC Capabilities Information
    return size;
}

void
EhtCapabilities::SetMaxMpduLength(uint16_t length)
{
    switch (length)
    {
    case 3895:
        m_macCapabilities.maxMpduLength = 0;
        break;
    case 7991:
        m_macCapabilities.maxMpduLength = 1;
        break;
    case 11454:
        m_macCapabilities.maxMpduLength = 2;
        break;
    default:
        NS_ABORT_MSG("Invalid MPDU Max Length value");
    }
}

uint16_t
EhtCapabilities::GetMaxMpduLength() const
{
    switch (m_macCapabilities.maxMpduLength)
    {
    case 0:
        return 3895;
    case 1:
        return 7991;
    case 2:
        return 11454;
    default:
        NS_ABORT_MSG("The value 3 is reserved");
    }
    return 0;
}

void
EhtCapabilities::SetMaxAmpduLength(uint32_t maxAmpduLength)
{
    for (uint8_t i = 0; i <= 1; i++)
    {
        if ((1UL << (23 + i)) - 1 == maxAmpduLength)
        {
            m_macCapabilities.maxAmpduLengthExponentExtension = i;
            return;
        }
    }
    NS_ABORT_MSG("Invalid A-MPDU Max Length value");
}

uint32_t
EhtCapabilities::GetMaxAmpduLength() const
{
    return std::min<uint32_t>((1UL << (23 + m_macCapabilities.maxAmpduLengthExponentExtension)) - 1,
                              15523200);
}

void
EhtCapabilities::SerializeInformationField(Buffer::Iterator start) const
{
    m_macCapabilities.Serialize(start);
}

uint16_t
EhtCapabilities::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    uint16_t count = 0;
    Buffer::Iterator i = start;
    count += m_macCapabilities.Deserialize(i);
    return count;
}

std::ostream&
operator<<(std::ostream& os, const EhtCapabilities& ehtCapabilities)
{
    // TODO
    return os;
}

} // namespace ns3
