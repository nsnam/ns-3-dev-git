/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
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
 * Author: Davide Magrin <davide@magr.in>
 */

#include "tim.h"

#include <algorithm>
#include <cstdint>

namespace ns3
{

WifiInformationElementId
Tim::ElementId() const
{
    return IE_TIM;
}

uint16_t
Tim::GetInformationFieldSize() const
{
    // When the TIM is carried in a non-S1G PPDU, in the event that all bits other than bit 0 in
    // the traffic indication virtual bitmap are 0, the Partial Virtual Bitmap field is encoded as
    // a single octet equal to 0, the Bitmap Offset subfield is 0, and the Length field is 4.
    // (Sec. 9.4.2.5.1 of 802.11-2020)
    // The size of the information field is the size of the Partial Virtual Bitmap field,
    // plus one octet each for the DTIM Count, DTIM Period, and Bitmap Control fields
    uint16_t partialVirtualBitmapSize =
        GetLastNonZeroOctetIndex() - GetPartialVirtualBitmapOffset() + 1;
    return partialVirtualBitmapSize + 3;
}

void
Tim::AddAid(uint16_t aid)
{
    NS_ABORT_IF(aid > 2007);

    m_aidValues.insert(aid);
}

bool
Tim::HasAid(uint16_t aid) const
{
    return m_aidValues.find(aid) != m_aidValues.end();
}

std::set<uint16_t>
Tim::GetAidSet(uint16_t aid) const
{
    auto start = m_aidValues.upper_bound(aid);
    return std::set<uint16_t>(start, m_aidValues.cend());
}

void
Tim::SerializeInformationField(Buffer::Iterator start) const
{
    start.WriteU8(m_dtimCount);
    start.WriteU8(m_dtimPeriod);

    // the Bitmap Control field is optional if the TIM is carried in an S1G PPDU, while
    // it is always present when the TIM is carried in a non-S1G PPDU
    start.WriteU8(GetBitmapControl());
    auto partialVirtualBitmap = GetPartialVirtualBitmap();
    for (auto byte : partialVirtualBitmap)
    {
        start.WriteU8(byte);
    }
}

uint16_t
Tim::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    NS_ABORT_MSG_IF(length < 2, "Invalid length: " << length);

    m_dtimCount = start.ReadU8();
    m_dtimPeriod = start.ReadU8();

    if (length == 2)
    {
        // no Bitmap Control field nor Partial Virtual Bitmap field
        return 2;
    }

    // Bitmap control field: here we determine the presence of multicast traffic and the offset
    auto bitmapControl = start.ReadU8();
    // Least significant bit is the Traffic Indication field
    m_hasMulticastPending = bitmapControl & 0x01;
    // Other bits are the Bitmap Offset
    uint8_t partialVirtualBitmapOffset = bitmapControl & 0xFE;
    // Next, deserialize the partial virtual bitmap
    uint16_t octetIndex;
    // length is the length of the information fields, so we need to
    // subtract 3 to get the length of the Partial Virtual Bitmap
    for (octetIndex = partialVirtualBitmapOffset;
         octetIndex < static_cast<uint16_t>(partialVirtualBitmapOffset + length - 3);
         ++octetIndex)
    {
        if (auto octet = start.ReadU8(); octet > 0)
        {
            // Look for bits set to 1
            for (uint8_t position = 0; position < 8; position++)
            {
                if ((octet >> position) & 0x1)
                {
                    m_aidValues.insert(GetAidFromOctetIndexAndBitPosition(octetIndex, position));
                }
            }
        }
    }
    return 3 + octetIndex - partialVirtualBitmapOffset;
}

uint8_t
Tim::GetAidOctetIndex(uint16_t aid) const
{
    // bit number N (0 <= N <= 2007) in the bitmap corresponds to bit number (N mod 8) in octet
    // number |_N / 8_| where the low order bit of each octet is bit number 0, and the high order
    // bit is bit number 7 (Sec. 9.4.2.5.1 of 802.11-2020)
    return (aid >> 3) & 0xff;
}

uint8_t
Tim::GetAidBit(uint16_t aid) const
{
    // bit number N (0 <= N <= 2007) in the bitmap corresponds to bit number (N mod 8) in octet
    // number |_N / 8_| where the low order bit of each octet is bit number 0, and the high order
    // bit is bit number 7 (Sec. 9.4.2.5.1 of 802.11-2020)
    return 0x01 << (aid & 0x07);
}

uint16_t
Tim::GetAidFromOctetIndexAndBitPosition(uint16_t octet, uint8_t position) const
{
    return (octet << 3) + position;
}

uint8_t
Tim::GetPartialVirtualBitmapOffset() const
{
    if (m_aidValues.empty())
    {
        return 0;
    }
    // N1 is the largest even number such that bits numbered 1 to (N1 * 8) – 1 in the traffic
    // indication virtual bitmap are all 0 (Sec. 9.4.2.5.1 of 802.11-2020).
    // Examples:
    // first bit set = 53, which belongs to octet 53 / 8 = 6 -> N1 = 6 (all bits 1 - 47 are zero)
    // first bit set = 61, which belongs to octet 61 / 8 = 7 -> N1 = 6 (all bits 1 - 47 are zero)
    return GetAidOctetIndex(*m_aidValues.cbegin()) & 0xFE;
}

uint8_t
Tim::GetLastNonZeroOctetIndex() const
{
    if (m_aidValues.empty())
    {
        return 0;
    }
    // N2 is the smallest number such that bits numbered (N2 + 1) * 8 to 2007 in the traffic
    // indication virtual bitmap are all 0 (Sec. 9.4.2.5.1 of 802.11-2020).
    // Examples:
    // last bit set = 53, which belongs to octet 53 / 8 = 6 -> N2 = 6 (all bits 56 - 2007 are zero)
    // last bit set = 61, which belongs to octet 61 / 8 = 7 -> N2 = 7 (all bits 64 - 2007 are zero)
    return GetAidOctetIndex(*m_aidValues.rbegin());
}

uint8_t
Tim::GetBitmapControl() const
{
    // Note that setting the bitmapControl directly as the offset can be done because the least
    // significant bit of the output of GetPartialVirtualBitmapOffset will always be zero, so we
    // are already putting the relevant information in the appropriate part of the byte.
    auto bitmapControl = GetPartialVirtualBitmapOffset();

    // Set the multicast indication bit, if this is a DTIM
    if ((m_dtimCount == 0) && m_hasMulticastPending)
    {
        bitmapControl |= 0x01;
    }

    return bitmapControl;
}

std::vector<uint8_t>
Tim::GetPartialVirtualBitmap() const
{
    auto offset = GetPartialVirtualBitmapOffset(); // N1

    // the Partial Virtual Bitmap field consists of octets numbered N1 to N2 of the traffic
    // indication virtual bitmap (Sec. 9.4.2.5.1 of 802.11-2020)
    std::vector<uint8_t> partialVirtualBitmap(GetLastNonZeroOctetIndex() - offset + 1, 0);

    for (auto aid : m_aidValues)
    {
        partialVirtualBitmap.at(GetAidOctetIndex(aid) - offset) |= GetAidBit(aid);
    }

    return partialVirtualBitmap;
}

void
Tim::Print(std::ostream& os) const
{
    os << "DTIM Count: " << +m_dtimCount << ", "
       << "DTIM Period: " << +m_dtimPeriod << ", "
       << "Has Multicast Pending: " << m_hasMulticastPending << ", AID values:";
    for (uint16_t aid = 0; aid < 2008; ++aid)
    {
        if (HasAid(aid))
        {
            os << aid << " ";
        }
    }
}

} // namespace ns3
