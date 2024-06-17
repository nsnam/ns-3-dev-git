/*
 * Copyright (c) 2011 Adrian Sai-wah Tam
 * Copyright (c) 2015 ResiliNets, ITTC, University of Kansas
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Original Author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 * Documentation, test cases: Truc Anh N. Nguyen   <annguyen@ittc.ku.edu>
 *                            ResiliNets Research Group   https://resilinets.org/
 *                            The University of Kansas
 *                            James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 */

#include "tcp-option-sack.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpOptionSack");

NS_OBJECT_ENSURE_REGISTERED(TcpOptionSack);

TcpOptionSack::TcpOptionSack()
    : TcpOption()
{
}

TcpOptionSack::~TcpOptionSack()
{
}

TypeId
TcpOptionSack::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpOptionSack")
                            .SetParent<TcpOption>()
                            .SetGroupName("Internet")
                            .AddConstructor<TcpOptionSack>();
    return tid;
}

TypeId
TcpOptionSack::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
TcpOptionSack::Print(std::ostream& os) const
{
    os << "blocks: " << GetNumSackBlocks() << ",";
    for (auto it = m_sackList.begin(); it != m_sackList.end(); ++it)
    {
        os << "[" << it->first << "," << it->second << "]";
    }
}

uint32_t
TcpOptionSack::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    NS_LOG_LOGIC("Serialized size: " << 2 + GetNumSackBlocks() * 8);
    return 2 + GetNumSackBlocks() * 8;
}

void
TcpOptionSack::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    Buffer::Iterator i = start;
    i.WriteU8(GetKind()); // Kind
    auto length = static_cast<uint8_t>(GetNumSackBlocks() * 8 + 2);
    i.WriteU8(length); // Length

    for (auto it = m_sackList.begin(); it != m_sackList.end(); ++it)
    {
        SequenceNumber32 leftEdge = it->first;
        SequenceNumber32 rightEdge = it->second;
        i.WriteHtonU32(leftEdge.GetValue());  // Left edge of the block
        i.WriteHtonU32(rightEdge.GetValue()); // Right edge of the block
    }
}

uint32_t
TcpOptionSack::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    Buffer::Iterator i = start;
    uint8_t readKind = i.ReadU8();
    if (readKind != GetKind())
    {
        NS_LOG_WARN("Malformed SACK option, wrong type");
        return 0;
    }

    uint8_t size = i.ReadU8();
    NS_LOG_LOGIC("Size: " << static_cast<uint32_t>(size));
    m_sackList.clear();
    uint8_t sackCount = (size - 2) / 8;
    while (sackCount)
    {
        SequenceNumber32 leftEdge(i.ReadNtohU32());
        SequenceNumber32 rightEdge(i.ReadNtohU32());
        SackBlock s(leftEdge, rightEdge);
        AddSackBlock(s);
        sackCount--;
    }

    return GetSerializedSize();
}

uint8_t
TcpOptionSack::GetKind() const
{
    return TcpOption::SACK;
}

void
TcpOptionSack::AddSackBlock(SackBlock s)
{
    NS_LOG_FUNCTION(this);
    m_sackList.push_back(s);
}

uint32_t
TcpOptionSack::GetNumSackBlocks() const
{
    NS_LOG_FUNCTION(this);
    NS_LOG_LOGIC("Number of SACK blocks appended: " << m_sackList.size());
    return static_cast<uint32_t>(m_sackList.size());
}

void
TcpOptionSack::ClearSackList()
{
    m_sackList.clear();
}

TcpOptionSack::SackList
TcpOptionSack::GetSackList() const
{
    NS_LOG_FUNCTION(this);
    return m_sackList;
}

std::ostream&
operator<<(std::ostream& os, const TcpOptionSack& sackOption)
{
    std::stringstream ss;
    ss << "{";
    for (auto it = sackOption.m_sackList.begin(); it != sackOption.m_sackList.end(); ++it)
    {
        ss << *it;
    }
    ss << "}";
    os << ss.str();
    return os;
}

std::ostream&
operator<<(std::ostream& os, const TcpOptionSack::SackBlock& sackBlock)
{
    std::stringstream ss;
    ss << "[" << sackBlock.first << ";" << sackBlock.second << "]";
    os << ss.str();
    return os;
}

} // namespace ns3
