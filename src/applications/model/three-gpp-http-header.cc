/*
 * Copyright (c) 2015 Magister Solutions
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Budiarto Herman <budiarto.herman@magister.fi>
 *
 */

#include "three-gpp-http-header.h"

#include "ns3/log.h"
#include "ns3/packet.h"

#include <sstream>

NS_LOG_COMPONENT_DEFINE("ThreeGppHttpHeader");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(ThreeGppHttpHeader);

ThreeGppHttpHeader::ThreeGppHttpHeader()
    : Header(),
      m_contentType(NOT_SET),
      m_contentLength(0),
      m_clientTs(0),
      m_serverTs(0)
{
    NS_LOG_FUNCTION(this);
}

// static
TypeId
ThreeGppHttpHeader::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ThreeGppHttpHeader").SetParent<Header>().AddConstructor<ThreeGppHttpHeader>();
    return tid;
}

TypeId
ThreeGppHttpHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
ThreeGppHttpHeader::GetSerializedSize() const
{
    return 2 + 4 + 8 + 8;
}

void
ThreeGppHttpHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    start.WriteU16(m_contentType);
    start.WriteU32(m_contentLength);
    start.WriteU64(m_clientTs);
    start.WriteU64(m_serverTs);
}

uint32_t
ThreeGppHttpHeader::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    uint32_t bytesRead = 0;

    // First block of 2 bytes (content type)
    m_contentType = start.ReadU16();
    bytesRead += 2;

    // Second block of 4 bytes (content length)
    m_contentLength = start.ReadU32();
    bytesRead += 4;

    // Third block of 8 bytes (client time stamp)
    m_clientTs = start.ReadU64();
    bytesRead += 8;

    // Fourth block of 8 bytes (server time stamp)
    m_serverTs = start.ReadU64();
    bytesRead += 8;

    return bytesRead;
}

void
ThreeGppHttpHeader::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "(Content-Type: " << m_contentType << " Content-Length: " << m_contentLength
       << " Client TS: " << TimeStep(m_clientTs).As(Time::S)
       << " Server TS: " << TimeStep(m_serverTs).As(Time::S) << ")";
}

std::string
ThreeGppHttpHeader::ToString() const
{
    NS_LOG_FUNCTION(this);
    std::ostringstream oss;
    Print(oss);
    return oss.str();
}

void
ThreeGppHttpHeader::SetContentType(ThreeGppHttpHeader::ContentType_t contentType)
{
    NS_LOG_FUNCTION(this << static_cast<uint16_t>(contentType));
    switch (contentType)
    {
    case NOT_SET:
        m_contentType = 0;
        break;
    case MAIN_OBJECT:
        m_contentType = 1;
        break;
    case EMBEDDED_OBJECT:
        m_contentType = 2;
        break;
    default:
        NS_FATAL_ERROR("Unknown Content-Type: " << contentType);
        break;
    }
}

ThreeGppHttpHeader::ContentType_t
ThreeGppHttpHeader::GetContentType() const
{
    ContentType_t ret;
    switch (m_contentType)
    {
    case 0:
        ret = NOT_SET;
        break;
    case 1:
        ret = MAIN_OBJECT;
        break;
    case 2:
        ret = EMBEDDED_OBJECT;
        break;
    default:
        NS_FATAL_ERROR("Unknown Content-Type: " << m_contentType);
        break;
    }
    return ret;
}

void
ThreeGppHttpHeader::SetContentLength(uint32_t contentLength)
{
    NS_LOG_FUNCTION(this << contentLength);
    m_contentLength = contentLength;
}

uint32_t
ThreeGppHttpHeader::GetContentLength() const
{
    return m_contentLength;
}

void
ThreeGppHttpHeader::SetClientTs(Time clientTs)
{
    NS_LOG_FUNCTION(this << clientTs.As(Time::S));
    m_clientTs = clientTs.GetTimeStep();
}

Time
ThreeGppHttpHeader::GetClientTs() const
{
    return TimeStep(m_clientTs);
}

void
ThreeGppHttpHeader::SetServerTs(Time serverTs)
{
    NS_LOG_FUNCTION(this << serverTs.As(Time::S));
    m_serverTs = serverTs.GetTimeStep();
}

Time
ThreeGppHttpHeader::GetServerTs() const
{
    return TimeStep(m_serverTs);
}

} // namespace ns3
