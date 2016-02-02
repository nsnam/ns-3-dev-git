/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007-2009 Strasbourg University
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
 * Author: Sebastien Vincent <vincent@clarinet.u-strasbg.fr>
 */

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"

#include "ipv6-interface.h"
#include "ns3/net-device.h"
#include "loopback-net-device.h"
#include "ns3/mac16-address.h"
#include "ns3/mac64-address.h"
#include "ipv6-l3-protocol.h"
#include "icmpv6-l4-protocol.h"
#include "ndisc-cache.h"
#include "ns3/traffic-control-layer.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("Ipv6Interface");

NS_OBJECT_ENSURE_REGISTERED (Ipv6Interface);

TypeId Ipv6Interface::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::Ipv6Interface")
    .SetParent<Object> ()
    .SetGroupName ("Internet")
  ;
  return tid;
}

Ipv6Interface::Ipv6Interface ()
  : m_ifup (false),
    m_forwarding (true),
    m_metric (1),
    m_node (0),
    m_device (0),
    m_ndCache (0),
    m_curHopLimit (0),
    m_baseReachableTime (0),
    m_reachableTime (0),
    m_retransTimer (0)
{
  NS_LOG_FUNCTION (this);
}

Ipv6Interface::~Ipv6Interface ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void Ipv6Interface::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_node = 0;
  m_device = 0;
  m_ndCache = 0;
  Object::DoDispose ();
}

void Ipv6Interface::DoSetup ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_node == 0 || m_device == 0)
    {
      return;
    }

  /* set up link-local address */
  if (!DynamicCast<LoopbackNetDevice> (m_device)) /* no autoconf for ip6-localhost */
    {
      Address addr = GetDevice ()->GetAddress ();

      if (Mac64Address::IsMatchingType (addr))
        {
          Ipv6InterfaceAddress ifaddr = Ipv6InterfaceAddress (Ipv6Address::MakeAutoconfiguredLinkLocalAddress (Mac64Address::ConvertFrom (addr)), Ipv6Prefix (64));
          AddAddress (ifaddr);
          m_linkLocalAddress = ifaddr;
        }
      else if (Mac48Address::IsMatchingType (addr))
        {
          Ipv6InterfaceAddress ifaddr = Ipv6InterfaceAddress (Ipv6Address::MakeAutoconfiguredLinkLocalAddress (Mac48Address::ConvertFrom (addr)), Ipv6Prefix (64));
          AddAddress (ifaddr);
          m_linkLocalAddress = ifaddr;
        }
      else if (Mac16Address::IsMatchingType (addr))
        {
          Ipv6InterfaceAddress ifaddr = Ipv6InterfaceAddress (Ipv6Address::MakeAutoconfiguredLinkLocalAddress (Mac16Address::ConvertFrom (addr)), Ipv6Prefix (64));
          AddAddress (ifaddr);
          m_linkLocalAddress = ifaddr;
        }
      else
        {
          NS_ASSERT_MSG (false, "IPv6 autoconf for this kind of address not implemented.");
        }
    }
  else
    {
      return; /* no NDISC cache for ip6-localhost */
    }

  Ptr<IpL4Protocol> proto = m_node->GetObject<Ipv6> ()->GetProtocol (Icmpv6L4Protocol::GetStaticProtocolNumber ());
  Ptr<Icmpv6L4Protocol> icmpv6;
  if (proto)
    {
      icmpv6 = proto->GetObject <Icmpv6L4Protocol> ();
    }
  if (icmpv6 && !m_ndCache)
    {
      m_ndCache = icmpv6->CreateCache (m_device, this);
    }
}

void Ipv6Interface::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  m_node = node;
  DoSetup ();
}

void Ipv6Interface::SetDevice (Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  m_device = device;
  DoSetup ();
}

Ptr<NetDevice> Ipv6Interface::GetDevice () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_device;
}

void Ipv6Interface::SetMetric (uint16_t metric)
{
  NS_LOG_FUNCTION (this << metric);
  m_metric = metric;
}

uint16_t Ipv6Interface::GetMetric () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_metric;
}

bool Ipv6Interface::IsUp () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ifup;
}

bool Ipv6Interface::IsDown () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return !m_ifup;
}

void Ipv6Interface::SetUp ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_ifup)
    {
      return;
    }
  DoSetup ();
  m_ifup = true;
}

void Ipv6Interface::SetDown ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_ifup = false;
  m_addresses.clear ();
  m_ndCache->Flush ();
}

bool Ipv6Interface::IsForwarding () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_forwarding;
}

void Ipv6Interface::SetForwarding (bool forwarding)
{
  NS_LOG_FUNCTION (this << forwarding);
  m_forwarding = forwarding;
}

bool Ipv6Interface::AddAddress (Ipv6InterfaceAddress iface)
{
  NS_LOG_FUNCTION_NOARGS ();
  Ipv6Address addr = iface.GetAddress ();

  /* DAD handling */
  if (!addr.IsAny ())
    {
      for (Ipv6InterfaceAddressListCI it = m_addresses.begin (); it != m_addresses.end (); ++it)
        {
          if (it->first.GetAddress () == addr)
            {
              return false;
            }
        }

      Ipv6Address solicited = Ipv6Address::MakeSolicitedAddress (iface.GetAddress ());
      m_addresses.push_back (std::make_pair (iface, solicited));

      if (!addr.IsAny () || !addr.IsLocalhost ())
        {
          /* DAD handling */
          Ptr<IpL4Protocol> proto = m_node->GetObject<Ipv6> ()->GetProtocol (Icmpv6L4Protocol::GetStaticProtocolNumber ());
          Ptr<Icmpv6L4Protocol> icmpv6;
          if (proto)
            {
              icmpv6 = proto->GetObject <Icmpv6L4Protocol> ();
            }

          if (icmpv6 && icmpv6->IsAlwaysDad ())
            {
              Simulator::Schedule (Seconds (0.), &Icmpv6L4Protocol::DoDAD, icmpv6, addr, this);
              Simulator::Schedule (Seconds (1.), &Icmpv6L4Protocol::FunctionDadTimeout, icmpv6, this, addr);
            }
        }
      return true;
    }

  /* bad address */
  return false;
}

Ipv6InterfaceAddress Ipv6Interface::GetLinkLocalAddress () const
{
  /* IPv6 interface has always at least one IPv6 link-local address */
  NS_LOG_FUNCTION_NOARGS ();

  return m_linkLocalAddress;
}

bool Ipv6Interface::IsSolicitedMulticastAddress (Ipv6Address address) const
{
  /* IPv6 interface has always at least one IPv6 Solicited Multicast address */
  NS_LOG_FUNCTION (this << address);

  for (Ipv6InterfaceAddressListCI it = m_addresses.begin (); it != m_addresses.end (); ++it)
     {
       if (it->second == address)
         {
           return true;
         }
     }

  return false;
}

Ipv6InterfaceAddress Ipv6Interface::GetAddress (uint32_t index) const
{
  NS_LOG_FUNCTION (this << index);
  uint32_t i = 0;

  if (m_addresses.size () > index)
    {
      for (Ipv6InterfaceAddressListCI it = m_addresses.begin (); it != m_addresses.end (); ++it)
        {
          if (i == index)
            {
              return it->first;
            }
          i++;
        }
    }

  NS_ASSERT_MSG (false, "Address " << index << " not found");
  Ipv6InterfaceAddress addr;
  return addr;  /* quiet compiler */
}

uint32_t Ipv6Interface::GetNAddresses () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_addresses.size ();
}

Ipv6InterfaceAddress Ipv6Interface::RemoveAddress (uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  uint32_t i = 0;

  if (m_addresses.size () < index)
    {
      NS_ASSERT_MSG (false, "Try to remove index that don't exist in Ipv6Interface::RemoveAddress");
    }

  for (Ipv6InterfaceAddressListI it = m_addresses.begin (); it != m_addresses.end (); ++it)
    {
      if (i == index)
        {
          Ipv6InterfaceAddress iface = it->first;
          m_addresses.erase (it);
          return iface;
        }

      i++;
    }

  NS_ASSERT_MSG (false, "Address " << index << " not found");
  Ipv6InterfaceAddress addr;
  return addr;  /* quiet compiler */
}

Ipv6InterfaceAddress 
Ipv6Interface::RemoveAddress(Ipv6Address address)
{
  NS_LOG_FUNCTION(this << address);

  if (address == address.GetLoopback())
    {
      NS_LOG_WARN ("Cannot remove loopback address.");
      return Ipv6InterfaceAddress();
    }

  for (Ipv6InterfaceAddressListI it = m_addresses.begin (); it != m_addresses.end (); ++it)
    {
      if(it->first.GetAddress () == address)
        {
          Ipv6InterfaceAddress iface = it->first;
          m_addresses.erase(it);
          return iface;
        }
    }
  return Ipv6InterfaceAddress();
}

Ipv6InterfaceAddress Ipv6Interface::GetAddressMatchingDestination (Ipv6Address dst)
{
  NS_LOG_FUNCTION (this << dst);

  for (Ipv6InterfaceAddressList::const_iterator it = m_addresses.begin (); it != m_addresses.end (); ++it)
    {
      Ipv6InterfaceAddress ifaddr = it->first;

      if (ifaddr.GetPrefix ().IsMatch (ifaddr.GetAddress (), dst))
        {
          return ifaddr;
        }
    }

  /*  NS_ASSERT_MSG (false, "Not matching address."); */
  Ipv6InterfaceAddress ret;
  return ret; /* quiet compiler */
}

void Ipv6Interface::Send (Ptr<Packet> p, const Ipv6Header & hdr, Ipv6Address dest)
{
  NS_LOG_FUNCTION (this << p << dest);

  if (!IsUp ())
    {
      return;
    }

  Ptr<Ipv6L3Protocol> ipv6 = m_node->GetObject<Ipv6L3Protocol> ();
  Ptr<TrafficControlLayer> tc = m_node->GetObject<TrafficControlLayer> ();

  NS_ASSERT (tc != 0);

  /* check if destination is localhost (::1), if yes we don't pass through
   * traffic control layer */
  if (DynamicCast<LoopbackNetDevice> (m_device))
    {
      /** \todo additional checks needed here (such as whether multicast
       * goes to loopback)?
       */
      p->AddHeader (hdr);
      m_device->Send (p, m_device->GetBroadcast (), Ipv6L3Protocol::PROT_NUMBER);
      return;
    }

  /* check if destination is for one of our interface */
  for (Ipv6InterfaceAddressListCI it = m_addresses.begin (); it != m_addresses.end (); ++it)
    {
      if (dest == it->first.GetAddress ())
        {
          p->AddHeader (hdr);
          tc->Receive (m_device, p, Ipv6L3Protocol::PROT_NUMBER,
                       m_device->GetBroadcast (),
                       m_device->GetBroadcast (),
                       NetDevice::PACKET_HOST);
          return;
        }
    }

  /* other address */
  if (m_device->NeedsArp ())
    {
      NS_LOG_LOGIC ("Needs ARP" << " " << dest);
      Ptr<Icmpv6L4Protocol> icmpv6 = ipv6->GetIcmpv6 ();
      Address hardwareDestination;
      bool found = false;

      NS_ASSERT (icmpv6);

      if (dest.IsMulticast ())
        {
          NS_LOG_LOGIC ("IsMulticast");
          NS_ASSERT_MSG (m_device->IsMulticast (), "Ipv6Interface::SendTo (): Sending multicast packet over non-multicast device");

          hardwareDestination = m_device->GetMulticast (dest);
          found = true;
        }
      else
        {
          NS_LOG_LOGIC ("NDISC Lookup");
          found = icmpv6->Lookup (p, hdr, dest, GetDevice (), m_ndCache, &hardwareDestination);
        }

      if (found)
        {
          NS_LOG_LOGIC ("Address Resolved.  Send.");
          tc->Send (m_device, p, hdr, hardwareDestination, Ipv6L3Protocol::PROT_NUMBER);
        }
    }
  else
    {
      NS_LOG_LOGIC ("Doesn't need ARP");
      tc->Send (m_device, p, hdr, m_device->GetBroadcast (), Ipv6L3Protocol::PROT_NUMBER);
    }
}

void Ipv6Interface::SetCurHopLimit (uint8_t curHopLimit)
{
  NS_LOG_FUNCTION (this << curHopLimit);
  m_curHopLimit = curHopLimit;
}

uint8_t Ipv6Interface::GetCurHopLimit () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_curHopLimit;
}

void Ipv6Interface::SetBaseReachableTime (uint16_t baseReachableTime)
{
  NS_LOG_FUNCTION (this << baseReachableTime);
  m_baseReachableTime = baseReachableTime;
}

uint16_t Ipv6Interface::GetBaseReachableTime () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_baseReachableTime;
}

void Ipv6Interface::SetReachableTime (uint16_t reachableTime)
{
  NS_LOG_FUNCTION (this << reachableTime);
  m_reachableTime = reachableTime;
}

uint16_t Ipv6Interface::GetReachableTime () const
{
  NS_LOG_FUNCTION_NOARGS (); 
  return m_reachableTime;
}

void Ipv6Interface::SetRetransTimer (uint16_t retransTimer)
{
  NS_LOG_FUNCTION (this << retransTimer); 
  m_retransTimer = retransTimer;
}

uint16_t Ipv6Interface::GetRetransTimer () const
{
  NS_LOG_FUNCTION_NOARGS (); 
  return m_retransTimer;
}

void Ipv6Interface::SetState (Ipv6Address address, Ipv6InterfaceAddress::State_e state)
{
  NS_LOG_FUNCTION (this << address << state);

  for (Ipv6InterfaceAddressListI it = m_addresses.begin (); it != m_addresses.end (); ++it)
    {
      if (it->first.GetAddress () == address)
        {
          it->first.SetState (state);
          return;
        }
    }
  /* not found, maybe address has expired */
}

void Ipv6Interface::SetNsDadUid (Ipv6Address address, uint32_t uid)
{
  NS_LOG_FUNCTION (this << address << uid);

  for (Ipv6InterfaceAddressListI it = m_addresses.begin (); it != m_addresses.end (); ++it)
    {
      if (it->first.GetAddress () == address)
        {
          it->first.SetNsDadUid (uid);
          return;
        }
    }
  /* not found, maybe address has expired */
}

Ptr<NdiscCache> Ipv6Interface::GetNdiscCache () const
{
  NS_LOG_FUNCTION (this);
  return m_ndCache;
}

} /* namespace ns3 */

