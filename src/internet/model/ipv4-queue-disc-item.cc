/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ipv4-queue-disc-item.h"

#include "tcp-header.h"
#include "udp-header.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4QueueDiscItem");

Ipv4QueueDiscItem::Ipv4QueueDiscItem(Ptr<Packet> p,
                                     const Address& addr,
                                     uint16_t protocol,
                                     const Ipv4Header& header)
    : QueueDiscItem(p, addr, protocol),
      m_header(header),
      m_headerAdded(false)
{
}

Ipv4QueueDiscItem::~Ipv4QueueDiscItem()
{
    NS_LOG_FUNCTION(this);
}

uint32_t
Ipv4QueueDiscItem::GetSize() const
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> p = GetPacket();
    NS_ASSERT(p);
    uint32_t ret = p->GetSize();
    if (!m_headerAdded)
    {
        ret += m_header.GetSerializedSize();
    }
    return ret;
}

const Ipv4Header&
Ipv4QueueDiscItem::GetHeader() const
{
    return m_header;
}

void
Ipv4QueueDiscItem::AddHeader()
{
    NS_LOG_FUNCTION(this);

    NS_ASSERT_MSG(!m_headerAdded, "The header has been already added to the packet");
    Ptr<Packet> p = GetPacket();
    NS_ASSERT(p);
    p->AddHeader(m_header);
    m_headerAdded = true;
}

void
Ipv4QueueDiscItem::Print(std::ostream& os) const
{
    if (!m_headerAdded)
    {
        os << m_header << " ";
    }
    os << GetPacket() << " "
       << "Dst addr " << GetAddress() << " "
       << "proto " << (uint16_t)GetProtocol() << " "
       << "txq " << (uint16_t)GetTxQueueIndex();
}

bool
Ipv4QueueDiscItem::Mark()
{
    NS_LOG_FUNCTION(this);
    if (!m_headerAdded && m_header.GetEcn() != Ipv4Header::ECN_NotECT)
    {
        m_header.SetEcn(Ipv4Header::ECN_CE);
        return true;
    }
    return false;
}

bool
Ipv4QueueDiscItem::GetUint8Value(QueueItem::Uint8Values field, uint8_t& value) const
{
    bool ret = false;

    switch (field)
    {
    case IP_DSFIELD:
        value = m_header.GetTos();
        ret = true;
        break;
    }

    return ret;
}

uint32_t
Ipv4QueueDiscItem::Hash(uint32_t perturbation) const
{
    NS_LOG_FUNCTION(this << perturbation);

    Ipv4Address src = m_header.GetSource();
    Ipv4Address dest = m_header.GetDestination();
    uint8_t prot = m_header.GetProtocol();
    uint16_t fragOffset = m_header.GetFragmentOffset();

    TcpHeader tcpHdr;
    UdpHeader udpHdr;
    uint16_t srcPort = 0;
    uint16_t destPort = 0;

    if (prot == 6 && fragOffset == 0) // TCP
    {
        GetPacket()->PeekHeader(tcpHdr);
        srcPort = tcpHdr.GetSourcePort();
        destPort = tcpHdr.GetDestinationPort();
    }
    else if (prot == 17 && fragOffset == 0) // UDP
    {
        GetPacket()->PeekHeader(udpHdr);
        srcPort = udpHdr.GetSourcePort();
        destPort = udpHdr.GetDestinationPort();
    }
    if (prot != 6 && prot != 17)
    {
        NS_LOG_WARN("Unknown transport protocol, no port number included in hash computation");
    }

    /* serialize the 5-tuple and the perturbation in buf */
    uint8_t buf[17];
    src.Serialize(buf);
    dest.Serialize(buf + 4);
    buf[8] = prot;
    buf[9] = (srcPort >> 8) & 0xff;
    buf[10] = srcPort & 0xff;
    buf[11] = (destPort >> 8) & 0xff;
    buf[12] = destPort & 0xff;
    buf[13] = (perturbation >> 24) & 0xff;
    buf[14] = (perturbation >> 16) & 0xff;
    buf[15] = (perturbation >> 8) & 0xff;
    buf[16] = perturbation & 0xff;

    // Linux calculates jhash2 (jenkins hash), we calculate murmur3 because it is
    // already available in ns-3
    uint32_t hash = Hash32((char*)buf, 17);

    NS_LOG_DEBUG("Hash value " << hash);

    return hash;
}

} // namespace ns3
