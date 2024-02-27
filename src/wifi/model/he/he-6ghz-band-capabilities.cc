/*
 * Copyright (c) 2024
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
 * Author: Rami Abdallah <abdallah.rami@gmail.com>
 */

#include "he-6ghz-band-capabilities.h"

namespace ns3
{

He6GhzBandCapabilities::He6GhzBandCapabilities()
    : m_capabilitiesInfo{}
{
}

WifiInformationElementId
He6GhzBandCapabilities::ElementId() const
{
    return IE_EXTENSION;
}

WifiInformationElementId
He6GhzBandCapabilities::ElementIdExt() const
{
    return IE_EXT_HE_6GHZ_CAPABILITIES;
}

uint16_t
He6GhzBandCapabilities::GetInformationFieldSize() const
{
    // Return size of Element ID Extension and
    // Capabilities Information field in octets
    return 1 + /* Element ID Ext */ +2 /* Capabilities Information */;
}

void
He6GhzBandCapabilities::SetMaxAmpduLength(uint32_t maxAmpduLength)
{
    for (uint8_t i = 0; i <= 7; i++)
    {
        if ((1UL << (13 + i)) - 1 == maxAmpduLength)
        {
            m_capabilitiesInfo.m_maxAmpduLengthExponent = i;
            return;
        }
    }
    NS_ABORT_MSG("Invalid A-MPDU Max Length value");
}

uint32_t
He6GhzBandCapabilities::GetMaxAmpduLength() const
{
    return (1UL << (13 + m_capabilitiesInfo.m_maxAmpduLengthExponent)) - 1;
}

void
He6GhzBandCapabilities::SetMaxMpduLength(uint16_t length)
{
    NS_ABORT_MSG_IF(length != 3895 && length != 7991 && length != 11454,
                    "Invalid MPDU Max Length value");
    if (length == 11454)
    {
        m_capabilitiesInfo.m_maxMpduLength = 2;
    }
    else if (length == 7991)
    {
        m_capabilitiesInfo.m_maxMpduLength = 1;
    }
    else
    {
        m_capabilitiesInfo.m_maxMpduLength = 0;
    }
}

uint16_t
He6GhzBandCapabilities::GetMaxMpduLength() const
{
    if (m_capabilitiesInfo.m_maxMpduLength == 0)
    {
        return 3895;
    }
    if (m_capabilitiesInfo.m_maxMpduLength == 1)
    {
        return 7991;
    }
    if (m_capabilitiesInfo.m_maxMpduLength == 2)
    {
        return 11454;
    }
    NS_ABORT_MSG("The value 3 is reserved");
}

void
He6GhzBandCapabilities::SerializeInformationField(Buffer::Iterator start) const
{
    uint16_t twoBytes = m_capabilitiesInfo.m_minMpduStartSpacing |
                        (m_capabilitiesInfo.m_maxAmpduLengthExponent << 3) |
                        (m_capabilitiesInfo.m_maxMpduLength << 6) |
                        (m_capabilitiesInfo.m_smPowerSave << 9) |
                        (m_capabilitiesInfo.m_rdResponder << 11) |
                        (m_capabilitiesInfo.m_rxAntennaPatternConsistency << 12) |
                        (m_capabilitiesInfo.m_txAntennaPatternConsistency << 13);
    start.WriteHtolsbU16(twoBytes);
}

uint16_t
He6GhzBandCapabilities::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator tmp = start;
    uint16_t twoBytes = start.ReadLsbtohU16();
    m_capabilitiesInfo.m_minMpduStartSpacing = twoBytes & 0x07;
    m_capabilitiesInfo.m_maxAmpduLengthExponent = (twoBytes >> 3) & 0x07;
    m_capabilitiesInfo.m_maxMpduLength = (twoBytes >> 6) & 0x03;
    m_capabilitiesInfo.m_smPowerSave = (twoBytes >> 9) & 0x03;
    m_capabilitiesInfo.m_rdResponder = (twoBytes >> 11) & 0x01;
    m_capabilitiesInfo.m_rxAntennaPatternConsistency = (twoBytes >> 12) & 0x01;
    m_capabilitiesInfo.m_txAntennaPatternConsistency = (twoBytes >> 13) & 0x01;
    return start.GetDistanceFrom(tmp);
}

void
He6GhzBandCapabilities::Print(std::ostream& os) const
{
    os << "HE 6GHz Band Capabilities=[Capabilities Information|"
       << " Min MPDU start spacing: " << +m_capabilitiesInfo.m_minMpduStartSpacing
       << " Max A-MPDU Length Exp: " << +m_capabilitiesInfo.m_maxAmpduLengthExponent
       << " Max MPDU Length: " << +m_capabilitiesInfo.m_maxMpduLength
       << " SM Power Save: " << +m_capabilitiesInfo.m_smPowerSave
       << " RD Responder: " << +m_capabilitiesInfo.m_rdResponder
       << " RX Antenna Pattern: " << +m_capabilitiesInfo.m_rxAntennaPatternConsistency
       << " TX Antenna Pattern: " << +m_capabilitiesInfo.m_txAntennaPatternConsistency << "]";
}

} // namespace ns3
