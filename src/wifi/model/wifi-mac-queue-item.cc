/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 *          Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "wifi-mac-queue-item.h"
#include "wifi-mac-trailer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiMacQueueItem");

WifiMacQueueItem::WifiMacQueueItem (Ptr<const Packet> p, const WifiMacHeader & header)
  : WifiMacQueueItem (p, header, Simulator::Now ())
{
}

WifiMacQueueItem::WifiMacQueueItem (Ptr<const Packet> p, const WifiMacHeader & header, Time tstamp)
  : m_packet (p),
    m_header (header),
    m_tstamp (tstamp)
{
}

WifiMacQueueItem::~WifiMacQueueItem ()
{
}

Ptr<const Packet>
WifiMacQueueItem::GetPacket (void) const
{
  return m_packet;
}

const WifiMacHeader&
WifiMacQueueItem::GetHeader (void) const
{
  return m_header;
}

WifiMacHeader&
WifiMacQueueItem::GetHeader (void)
{
  return m_header;
}

Mac48Address
WifiMacQueueItem::GetDestinationAddress (void) const
{
  return m_header.GetAddr1 ();
}

Time
WifiMacQueueItem::GetTimeStamp (void) const
{
  return m_tstamp;
}

uint32_t
WifiMacQueueItem::GetSize (void) const
{
  return m_packet->GetSize () + m_header.GetSerializedSize () + WIFI_MAC_FCS_LENGTH;
}

void
WifiMacQueueItem::Print (std::ostream& os) const
{
  os << "size=" << m_packet->GetSize ()
     << ", to=" << m_header.GetAddr1 ()
     << ", seqN=" << m_header.GetSequenceNumber ()
     << ", lifetime=" << (Simulator::Now () - m_tstamp).GetMicroSeconds () << "us";
  if (m_header.IsQosData ())
    {
      os << ", tid=" << +m_header.GetQosTid ();
      if (m_header.IsQosNoAck ())
        {
          os << ", ack=NoAck";
        }
      else if (m_header.IsQosAck ())
        {
          os << ", ack=NormalAck";
        }
      else if (m_header.IsQosBlockAck ())
        {
          os << ", ack=BlockAck";
        }
    }
}

std::ostream & operator << (std::ostream &os, const WifiMacQueueItem &item)
{
  item.Print (os);
  return os;
}

} //namespace ns3
