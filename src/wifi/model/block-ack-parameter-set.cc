/*
 * Copyright (c) 2026 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "block-ack-parameter-set.h"

namespace
{
/// Size in bytes of Block Ack Parameter Set field, IEEE 802.11-2024 Figure 9-151
constexpr uint32_t WIFI_BLOCK_ACK_PARAMETER_SIZE_B = 2;
} // namespace

namespace ns3
{

uint32_t
BlockAckParameterSet::GetSerializedSize() const
{
    return WIFI_BLOCK_ACK_PARAMETER_SIZE_B;
}

Buffer::Iterator
BlockAckParameterSet::Serialize(Buffer::Iterator start) const
{
    uint16_t params = m_amsduSupport ? 1 : 0;
    params |= (m_policy & 0x01) << 1;
    params |= (m_tid & 0x0f) << 2;
    params |= (m_bufferSize & 0x03ff) << 6;
    start.WriteU16(params);
    return start;
}

Buffer::Iterator
BlockAckParameterSet::Deserialize(Buffer::Iterator start)
{
    const auto params = start.ReadU16();
    m_amsduSupport = ((params & 0x01) == 1);
    m_policy = (params >> 1) & 0x01;
    m_tid = (params >> 2) & 0x0f;
    m_bufferSize = (params >> 6) & 0x03ff;
    return start;
}

std::ostream&
operator<<(std::ostream& os, const BlockAckParameterSet& params)
{
    os << "A-MSDU support=" << params.m_amsduSupport << " Policy=" << +params.m_policy
       << " TID=" << +params.m_tid << " Buffer size=" << params.m_bufferSize;
    return os;
}

} // namespace ns3
