/*
 * Copyright (c) 2011 Adrian Sai-wah Tam
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Original Author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 * Documentation, test cases: Truc Anh N. Nguyen   <annguyen@ittc.ku.edu>
 *                            ResiliNets Research Group   https://resilinets.org/
 *                            The University of Kansas
 *                            James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 */

#include "tcp-option-sack-permitted.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpOptionSackPermitted");

NS_OBJECT_ENSURE_REGISTERED(TcpOptionSackPermitted);

TcpOptionSackPermitted::TcpOptionSackPermitted()
    : TcpOption()
{
}

TcpOptionSackPermitted::~TcpOptionSackPermitted()
{
}

TypeId
TcpOptionSackPermitted::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpOptionSackPermitted")
                            .SetParent<TcpOption>()
                            .SetGroupName("Internet")
                            .AddConstructor<TcpOptionSackPermitted>();
    return tid;
}

TypeId
TcpOptionSackPermitted::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
TcpOptionSackPermitted::Print(std::ostream& os) const
{
    os << "[sack_perm]";
}

uint32_t
TcpOptionSackPermitted::GetSerializedSize() const
{
    return 2;
}

void
TcpOptionSackPermitted::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(GetKind()); // Kind
    i.WriteU8(2);         // Length
}

uint32_t
TcpOptionSackPermitted::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    uint8_t readKind = i.ReadU8();
    if (readKind != GetKind())
    {
        NS_LOG_WARN("Malformed Sack-Permitted option");
        return 0;
    }

    uint8_t size = i.ReadU8();
    if (size != 2)
    {
        NS_LOG_WARN("Malformed Sack-Permitted option");
        return 0;
    }
    return GetSerializedSize();
}

uint8_t
TcpOptionSackPermitted::GetKind() const
{
    return TcpOption::SACKPERMITTED;
}

} // namespace ns3
