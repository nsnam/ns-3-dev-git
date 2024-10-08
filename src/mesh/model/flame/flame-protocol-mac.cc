/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kirill Andreev <andreev@iitp.ru>
 */

#include "flame-protocol-mac.h"

#include "flame-header.h"
#include "flame-protocol.h"

#include "ns3/log.h"
#include "ns3/wifi-mac-header.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FlameProtocolMac");

namespace flame
{

FlameProtocolMac::FlameProtocolMac(Ptr<FlameProtocol> protocol)
    : m_protocol(protocol)
{
}

FlameProtocolMac::~FlameProtocolMac()
{
    m_protocol = nullptr;
    m_parent = nullptr;
}

void
FlameProtocolMac::SetParent(Ptr<MeshWifiInterfaceMac> parent)
{
    m_parent = parent;
}

bool
FlameProtocolMac::Receive(Ptr<Packet> packet, const WifiMacHeader& header)
{
    if (!header.IsData())
    {
        return true;
    }
    FlameTag tag;
    if (packet->PeekPacketTag(tag))
    {
        NS_FATAL_ERROR("FLAME tag is not supposed to be received by network");
    }
    tag.receiver = header.GetAddr1();
    tag.transmitter = header.GetAddr2();
    if (tag.receiver == Mac48Address::GetBroadcast())
    {
        m_stats.rxBroadcast++;
    }
    else
    {
        m_stats.rxUnicast++;
    }
    m_stats.rxBytes += packet->GetSize();
    packet->AddPacketTag(tag);
    return true;
}

bool
FlameProtocolMac::UpdateOutcomingFrame(Ptr<Packet> packet,
                                       WifiMacHeader& header,
                                       Mac48Address from,
                                       Mac48Address to)
{
    if (!header.IsData())
    {
        return true;
    }
    FlameTag tag;
    if (!packet->RemovePacketTag(tag))
    {
        NS_FATAL_ERROR("FLAME tag must exist here");
    }
    header.SetAddr1(tag.receiver);
    if (tag.receiver == Mac48Address::GetBroadcast())
    {
        m_stats.txBroadcast++;
    }
    else
    {
        m_stats.txUnicast++;
    }
    m_stats.txBytes += packet->GetSize();
    return true;
}

uint16_t
FlameProtocolMac::GetChannelId() const
{
    return m_parent->GetFrequencyChannel();
}

FlameProtocolMac::Statistics::Statistics()
    : txUnicast(0),
      txBroadcast(0),
      txBytes(0),
      rxUnicast(0),
      rxBroadcast(0),
      rxBytes(0)
{
}

void
FlameProtocolMac::Statistics::Print(std::ostream& os) const
{
    os << "<Statistics "
          "txUnicast=\""
       << txUnicast
       << "\" "
          "txBroadcast=\""
       << txBroadcast
       << "\" "
          "txBytes=\""
       << txBytes
       << "\" "
          "rxUnicast=\""
       << rxUnicast
       << "\" "
          "rxBroadcast=\""
       << rxBroadcast
       << "\" "
          "rxBytes=\""
       << rxBytes << "\"/>" << std::endl;
}

void
FlameProtocolMac::Report(std::ostream& os) const
{
    os << "<FlameProtocolMac" << std::endl
       << "address =\"" << m_parent->GetAddress() << "\">" << std::endl;
    m_stats.Print(os);
    os << "</FlameProtocolMac>" << std::endl;
}

void
FlameProtocolMac::ResetStats()
{
    m_stats = Statistics();
}

} // namespace flame
} // namespace ns3
