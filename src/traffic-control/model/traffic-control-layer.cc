/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Natale Patriciello <natale.patriciello@gmail.com>
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
 */

#include "traffic-control-layer.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/queue-disc.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TrafficControlLayer");

NS_OBJECT_ENSURE_REGISTERED (TrafficControlLayer);

TypeId
TrafficControlLayer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TrafficControlLayer")
    .SetParent<Object> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<TrafficControlLayer> ()
  ;
  return tid;
}

TypeId
TrafficControlLayer::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

TrafficControlLayer::TrafficControlLayer ()
  : Object ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
TrafficControlLayer::RegisterProtocolHandler (Node::ProtocolHandler handler,
                                              uint16_t protocolType, Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION (this << protocolType << device);

  struct Node::ProtocolHandlerEntry entry;
  entry.handler = handler;
  entry.protocol = protocolType;
  entry.device = device;
  entry.promiscuous = false;

  m_handlers.push_back (entry);

  NS_LOG_DEBUG ("Handler for NetDevice: " << device << " registered for protocol " <<
                protocolType << ".");
}

void
TrafficControlLayer::Receive (Ptr<NetDevice> device, Ptr<const Packet> p,
                              uint16_t protocol, const Address &from, const Address &to,
                              NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION (this << device << p << protocol << from << to << packetType);

  bool found = false;

  for (Node::ProtocolHandlerList::iterator i = m_handlers.begin ();
       i != m_handlers.end (); i++)
    {
      if (i->device == 0
          || (i->device != 0 && i->device == device))
        {
          if (i->protocol == 0
              || i->protocol == protocol)
            {
              NS_LOG_DEBUG ("Found handler for packet " << p << ", protocol " <<
                            protocol << " and NetDevice " << device <<
                            ". Send packet up");
              i->handler (device, p, protocol, from, to, packetType);
              found = true;
            }
        }
    }

  if (! found)
    {
      NS_FATAL_ERROR ("Handler for protocol " << p << " and device " << device <<
                      " not found. It isn't forwarded up; it dies here.");
    }
}

void
TrafficControlLayer::Send (Ptr<NetDevice> device, Ptr<Packet> packet,  const Header & hdr,
                           const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << device << packet << dest << protocolNumber);

  NS_LOG_DEBUG ("Send packet to device " << device << " protocol number " <<
                protocolNumber);

  // determine the transmission queue of the device where the packet will be enqueued
  uint8_t txq = device->GetSelectedQueue (packet);

  Ptr<QueueDisc> qDisc = device->GetObject<QueueDisc> ();

  if (qDisc == 0)
    {
      // The device has no attached queue disc, thus add the header to the packet and
      // send it directly to the device if the selected queue is not stopped
      if (!device->GetTxQueue (txq)->IsStopped ())
        {
          packet->AddHeader (hdr);
          device->Send (packet, dest, protocolNumber);
        }
    }
  else
    {
      // determine the header type (IPv4 or IPv6) to create the queue disc item
      Ptr<QueueDiscItem> item;
      try
        {
          const Ipv4Header & ipv4Header = dynamic_cast<const Ipv4Header &> (hdr);
          item = Create<QueueDiscItem> (packet, ipv4Header, dest, protocolNumber, txq);
        }
      catch (std::bad_cast)
        {
          try
            {
              const Ipv6Header & ipv6Header = dynamic_cast<const Ipv6Header &> (hdr);
              item = Create<QueueDiscItem> (packet, ipv6Header, dest, protocolNumber, txq);
            }
          catch (std::bad_cast)
            {
              NS_ASSERT (false);
            }
        }

      qDisc->Enqueue (item);
      qDisc->Run ();
    }
}

} // namespace ns3
