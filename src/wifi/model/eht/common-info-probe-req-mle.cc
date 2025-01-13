/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sharan Naribole <sharan.naribole@gmail.com>
 */

#include "common-info-probe-req-mle.h"

#include "ns3/abort.h"

namespace ns3
{

uint16_t
CommonInfoProbeReqMle::GetPresenceBitmap() const
{
    // IEEE 802.11be D5.0 Figure 9-1072q
    return (m_apMldId.has_value() ? 0x0001 : 0x0);
}

uint8_t
CommonInfoProbeReqMle::GetSize() const
{
    uint8_t size = 1; // Common Info Length
    size += (m_apMldId.has_value() ? 1 : 0);
    return size;
}

void
CommonInfoProbeReqMle::Serialize(Buffer::Iterator& start) const
{
    start.WriteU8(GetSize()); // Common Info Length
    if (m_apMldId.has_value())
    {
        start.WriteU8(*m_apMldId);
    }
}

uint8_t
CommonInfoProbeReqMle::Deserialize(Buffer::Iterator start, uint16_t presence)
{
    auto i = start;
    auto length = i.ReadU8();
    uint8_t count = 1;

    if (presence & 0x1)
    {
        m_apMldId = i.ReadU8();
        count++;
    }
    NS_ABORT_MSG_IF(count != length,
                    "Common Info Length (" << +length
                                           << ") differs "
                                              "from actual number of bytes read ("
                                           << +count << ")");
    return count;
}

} // namespace ns3
