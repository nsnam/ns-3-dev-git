/*
 * Copyright (c) 2017 Sébastien Deronne
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

#include "he-operation.h"

namespace ns3
{

HeOperation::HeOperation()
    : m_bssColor(0),
      m_defaultPEDuration(0),
      m_twtRequired(0),
      m_heDurationBasedRtsThreshold(0),
      m_partialBssColor(0),
      m_maxBssidIndicator(0),
      m_txBssidIndicator(0),
      m_bssColorDisabled(0),
      m_dualBeacon(0),
      m_basicHeMcsAndNssSet(0xfffc)
{
}

WifiInformationElementId
HeOperation::ElementId() const
{
    return IE_EXTENSION;
}

WifiInformationElementId
HeOperation::ElementIdExt() const
{
    return IE_EXT_HE_OPERATION;
}

void
HeOperation::Print(std::ostream& os) const
{
    os << "HE Operation=" << GetHeOperationParameters() << "|" << GetBasicHeMcsAndNssSet();
}

uint16_t
HeOperation::GetInformationFieldSize() const
{
    return 7;
}

void
HeOperation::SetHeOperationParameters(uint32_t ctrl)
{
    m_bssColor = ctrl & 0x3f;
    m_defaultPEDuration = (ctrl >> 6) & 0x07;
    m_twtRequired = (ctrl >> 9) & 0x01;
    m_heDurationBasedRtsThreshold = (ctrl >> 10) & 0x03ff;
    m_partialBssColor = (ctrl >> 20) & 0x01;
    m_maxBssidIndicator = (ctrl >> 21) & 0xff;
    m_txBssidIndicator = (ctrl >> 29) & 0x01;
    m_bssColorDisabled = (ctrl >> 30) & 0x01;
    m_dualBeacon = (ctrl >> 31) & 0x01;
}

uint32_t
HeOperation::GetHeOperationParameters() const
{
    uint32_t val = 0;
    val |= m_bssColor & 0x3f;
    val |= (m_defaultPEDuration & 0x07) << 6;
    val |= (m_twtRequired & 0x01) << 9;
    val |= (m_heDurationBasedRtsThreshold & 0x03ff) << 10;
    val |= (m_partialBssColor & 0x01) << 20;
    val |= (m_maxBssidIndicator & 0xff) << 21;
    val |= (m_txBssidIndicator & 0x01) << 29;
    val |= (m_bssColorDisabled & 0x01) << 30;
    val |= (m_dualBeacon & 0x01) << 31;
    return val;
}

void
HeOperation::SetMaxHeMcsPerNss(uint8_t nss, uint8_t maxHeMcs)
{
    NS_ASSERT((maxHeMcs >= 7 && maxHeMcs <= 11) && (nss >= 1 && nss <= 8));

    // IEEE 802.11ax-2021 9.4.2.248.4 Supported HE-MCS And NSS Set field
    uint8_t val = 0x03; // not supported
    if (maxHeMcs > 9)   // MCS 0 - 11
    {
        val = 0x02;
    }
    else if (maxHeMcs > 7) // MCS 0 - 9
    {
        val = 0x01;
    }
    else if (maxHeMcs == 7) // MCS 0 - 7
    {
        val = 0x01;
    }

    // clear bits for that nss
    const uint16_t mask = ~(0x03 << ((nss - 1) * 2));
    m_basicHeMcsAndNssSet &= mask;

    // update bits for that nss
    m_basicHeMcsAndNssSet |= ((val & 0x03) << ((nss - 1) * 2));
}

uint16_t
HeOperation::GetBasicHeMcsAndNssSet() const
{
    return m_basicHeMcsAndNssSet;
}

void
HeOperation::SetBssColor(uint8_t bssColor)
{
    NS_ABORT_UNLESS(bssColor < 64); // 6 bits
    m_bssColor = bssColor;
    m_bssColorDisabled = 0;
}

uint8_t
HeOperation::GetBssColor() const
{
    return m_bssColor;
}

void
HeOperation::SerializeInformationField(Buffer::Iterator start) const
{
    // write the corresponding value for each bit
    start.WriteHtolsbU32(GetHeOperationParameters());
    start.WriteU16(GetBasicHeMcsAndNssSet());
    // todo: VHT Operation Information (variable)
}

uint16_t
HeOperation::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;
    uint32_t heOperationParameters = i.ReadLsbtohU32();
    m_basicHeMcsAndNssSet = i.ReadU16();
    SetHeOperationParameters(heOperationParameters);
    // todo: VHT Operation Information (variable)
    return length;
}

} // namespace ns3
