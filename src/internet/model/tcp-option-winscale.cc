/*
 * Copyright (c) 2011 Adrian Sai-wah Tam
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 * Documentation, test cases: Natale Patriciello <natale.patriciello@gmail.com>
 */

#include "tcp-option-winscale.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpOptionWinScale");

NS_OBJECT_ENSURE_REGISTERED(TcpOptionWinScale);

TcpOptionWinScale::TcpOptionWinScale()
    : TcpOption(),
      m_scale(0)
{
}

TcpOptionWinScale::~TcpOptionWinScale()
{
}

TypeId
TcpOptionWinScale::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpOptionWinScale")
                            .SetParent<TcpOption>()
                            .SetGroupName("Internet")
                            .AddConstructor<TcpOptionWinScale>();
    return tid;
}

TypeId
TcpOptionWinScale::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
TcpOptionWinScale::Print(std::ostream& os) const
{
    os << static_cast<int>(m_scale);
}

uint32_t
TcpOptionWinScale::GetSerializedSize() const
{
    return 3;
}

void
TcpOptionWinScale::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(GetKind()); // Kind
    i.WriteU8(3);         // Length
    i.WriteU8(m_scale);   // Max segment size
}

uint32_t
TcpOptionWinScale::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    uint8_t readKind = i.ReadU8();
    if (readKind != GetKind())
    {
        NS_LOG_WARN("Malformed Window Scale option");
        return 0;
    }
    uint8_t size = i.ReadU8();
    if (size != 3)
    {
        NS_LOG_WARN("Malformed Window Scale option");
        return 0;
    }
    m_scale = i.ReadU8();
    return GetSerializedSize();
}

uint8_t
TcpOptionWinScale::GetKind() const
{
    return TcpOption::WINSCALE;
}

uint8_t
TcpOptionWinScale::GetScale() const
{
    NS_ASSERT(m_scale <= 14);

    return m_scale;
}

void
TcpOptionWinScale::SetScale(uint8_t scale)
{
    NS_ASSERT(scale <= 14);

    m_scale = scale;
}

} // namespace ns3
