/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 UPB
 * Copyright (c) 2017 NITK Surathkal
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
 * Author: Radu Lupu <rlupu@elcom.pub.ro>
 *         Ankit Deepak <adadeepak8@gmail.com>
 *         Deepti Rajagopal <deeptir96@gmail.com>
 *
 */

#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-packet-info-tag.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/config.h"
#include "dhcp-server.h"
#include "dhcp-header.h"
#include "ns3/ipv4.h"
#include <map>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DhcpServer");
NS_OBJECT_ENSURE_REGISTERED (DhcpServer);

TypeId
DhcpServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DhcpServer")
    .SetParent<Application> ()
    .AddConstructor<DhcpServer> ()
    .SetGroupName ("Internet-Apps")
    .AddAttribute ("LeaseTime",
                   "Lease for which address will be leased.",
                   TimeValue (Seconds (30)),
                   MakeTimeAccessor (&DhcpServer::m_lease),
                   MakeTimeChecker ())
    .AddAttribute ("RenewTime",
                   "Time after which client should renew.",
                   TimeValue (Seconds (15)),
                   MakeTimeAccessor (&DhcpServer::m_renew),
                   MakeTimeChecker ())
    .AddAttribute ("RebindTime",
                   "Time after which client should rebind.",
                   TimeValue (Seconds (25)),
                   MakeTimeAccessor (&DhcpServer::m_rebind),
                   MakeTimeChecker ())
    .AddAttribute ("PoolAddresses",
                   "Pool of addresses to provide on request.",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&DhcpServer::m_poolAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("FirstAddress",
                   "The First valid address that can be given.",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&DhcpServer::m_minAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("LastAddress",
                   "The Last valid address that can be given.",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&DhcpServer::m_maxAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("PoolMask",
                   "Mask of the pool of addresses.",
                   Ipv4MaskValue (),
                   MakeIpv4MaskAccessor (&DhcpServer::m_poolMask),
                   MakeIpv4MaskChecker ())
    .AddAttribute ("Gateway",
                   "Address of default gateway",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&DhcpServer::m_gateway),
                   MakeIpv4AddressChecker ())
  ;
  return tid;
}

DhcpServer::DhcpServer ()
{
  NS_LOG_FUNCTION (this);
  m_nextAddressSeq = 1;
}

DhcpServer::~DhcpServer ()
{
  NS_LOG_FUNCTION (this);
}

void
DhcpServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void DhcpServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_minAddress < m_maxAddress,"Invalid Address range");
  m_usedRange = 0;
  if (m_socket == 0)
    {
      uint32_t addrIndex;
      int32_t ifIndex;

      //add the DHCP local address to the leased addresses list, if it is defined!
      Ptr<Ipv4> ipv4 = GetNode ()->GetObject<Ipv4> ();
      ifIndex = ipv4->GetInterfaceForPrefix (m_poolAddress, m_poolMask);
      if (ifIndex >= 0)
        {
          for (addrIndex = 0; addrIndex < ipv4->GetNAddresses (ifIndex); addrIndex++)
            {
              if ((ipv4->GetAddress (ifIndex, addrIndex).GetLocal ().Get () & m_poolMask.Get ()) == m_poolAddress.Get () &&
                  ipv4->GetAddress (ifIndex, addrIndex).GetLocal ().Get () >= m_minAddress.Get () &&
                  ipv4->GetAddress (ifIndex, addrIndex).GetLocal ().Get () <= m_maxAddress.Get ())
                {
                  m_usedRange++;
                  // set infinite GRANTED_LEASED_TIME for my address
                  m_leasedAddresses.insert (std::make_pair (std::make_pair (GetNode ()->GetDevice (ifIndex)->GetAddress (),
                                                                            ipv4->GetAddress (ifIndex, addrIndex).GetLocal ()), 0xffffffff));
                  break;
                }
            }
        }

      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), PORT);
      m_socket->SetAllowBroadcast (true);
      m_socket->Bind (local);
      m_socket->SetRecvPktInfo (true);
    }

  m_socket->SetRecvCallback (MakeCallback (&DhcpServer::NetHandler, this));
  m_expiredEvent = Simulator::Schedule (Seconds (1), &DhcpServer::TimerHandler, this);
}

void DhcpServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }

  m_leasedAddresses.clear ();
  Simulator::Remove (m_expiredEvent);
}

void DhcpServer::TimerHandler ()
{
  // Set up timeout events and release of unsolicited addresses from the list
  std::map<std::pair<Address, Ipv4Address>, uint32_t>::iterator i;
  for (i = m_leasedAddresses.begin (); i != m_leasedAddresses.end (); i++)
    {
      // update the address state
      if (i->second == 1)
        {
          NS_LOG_INFO ("Address leased state expired, address removed - " <<
                       "MAC: " << i->first.first <<
                       "IP address " << i->first.second);
          i->second = 0;
          m_usedRange--;
          continue;
        }
      if (i->second != 0xffffffff || i->second != 0)
        {
          i->second--;
        }
    }
  m_expiredEvent = Simulator::Schedule (Seconds (1), &DhcpServer::TimerHandler, this);
}

void DhcpServer::NetHandler (Ptr<Socket> socket)
{
  DhcpHeader header;
  Ptr<Packet> packet = 0;
  Address from;
  packet = m_socket->RecvFrom (from);

  InetSocketAddress senderAddr = InetSocketAddress::ConvertFrom (from);

  Ipv4PacketInfoTag interfaceInfo;
  if (!packet->RemovePacketTag (interfaceInfo))
    {
      NS_ABORT_MSG ("No incoming interface on DHCP message, aborting.");
    }
  uint32_t incomingIf = interfaceInfo.GetRecvIf ();
  Ptr<NetDevice> iDev = GetNode ()->GetDevice (incomingIf);

  if (packet->RemoveHeader (header) == 0)
    {
      return;
    }
  if (header.GetType () == DhcpHeader::DHCPDISCOVER)
    {
      SendOffer (iDev, header, senderAddr);
    }
  if (header.GetType () == DhcpHeader::DHCPREQ && (header.GetReq ()).Get () >= m_minAddress.Get () && (header.GetReq ()).Get () <= m_maxAddress.Get ())
    {
      SendAck (iDev, header, senderAddr);
    }
}

void DhcpServer::SendOffer (Ptr<NetDevice> iDev, DhcpHeader header, InetSocketAddress from)
{
  DhcpHeader newDhcpHeader;
  Ipv4Address address;
  Address source = header.GetChaddr ();
  uint32_t tran = header.GetTran ();
  Ptr<Packet> packet = 0;
  bool found = false;
  Ipv4Address foundAddr;

  NS_LOG_INFO ("DHCP DISCOVER from: " << from.GetIpv4 () << " source port: " <<  from.GetPort ());

  std::map<std::pair<Address, Ipv4Address>, uint32_t>::iterator i;
  for (i = m_leasedAddresses.begin (); i != m_leasedAddresses.end (); i++)
    {
      if (i->first.first == source)
        {
          foundAddr = i->first.second;
          found = true;
          m_leasedAddresses.erase (i--);
          break;
        }
    }
  if (!found)
    {
      if ((header.GetReq ()).Get () >= m_minAddress.Get () && (header.GetReq ()).Get () <= m_maxAddress.Get ())
        {
          for (i = m_leasedAddresses.begin (); i != m_leasedAddresses.end (); i++)
            {
              if (i->first.second == header.GetReq ())
                {
                  break;
                }
            }
          if (i == m_leasedAddresses.end ())
            {
              foundAddr = header.GetReq ();
              found = true;
            }
        }
    }
  if (found == false)
    {
      // find a new address to be leased

      // \todo: redo this from scratch - it doesn't work.
      if (m_usedRange <= m_maxAddress.Get () - m_minAddress.Get ())
        {
          for (i = m_leasedAddresses.begin (); i != m_leasedAddresses.end (); i++)
            {
              // check whether the address is occupied
              if (i->first.second.Get () - m_minAddress.Get () == m_nextAddressSeq && i->second != 0)
                {
                  m_nextAddressSeq = m_nextAddressSeq % (m_maxAddress.Get () - m_minAddress.Get ()) + 1;
                  i = m_leasedAddresses.begin ();
                }
              if (i->first.second.Get () - m_minAddress.Get () == m_nextAddressSeq && i->second == 0)
                {
                  m_leasedAddresses.erase (i--);
                }
            }
          if (i == m_leasedAddresses.end ())
            {
              foundAddr.Set (m_minAddress.Get () + m_nextAddressSeq);
              m_nextAddressSeq = m_nextAddressSeq % (m_maxAddress.Get () - m_minAddress.Get ()) + 1;
              found = true;
            }
        }

      // \todo - until here.
    }
  if (found)
    {
      m_usedRange++;
      m_leasedAddresses.insert (std::make_pair (std::make_pair (source, foundAddr), m_lease.GetSeconds ()));
      address = foundAddr;

      packet = Create<Packet> ();
      newDhcpHeader.ResetOpt ();
      newDhcpHeader.SetType (DhcpHeader::DHCPOFFER);
      newDhcpHeader.SetChaddr (source);
      newDhcpHeader.SetYiaddr (address);

      Ptr<Ipv4> ipv4 = GetNode ()->GetObject<Ipv4> ();
      Ipv4Address myAddress = ipv4->SelectSourceAddress (iDev, Ipv4Address ("255.255.255.255"), Ipv4InterfaceAddress::InterfaceAddressScope_e::GLOBAL);

      newDhcpHeader.SetDhcps (myAddress);
      newDhcpHeader.SetMask (m_poolMask.Get ());
      newDhcpHeader.SetTran (tran);
      newDhcpHeader.SetLease (m_lease.GetSeconds ());
      newDhcpHeader.SetRenew (m_renew.GetSeconds ());
      newDhcpHeader.SetRebind (m_rebind.GetSeconds ());
      newDhcpHeader.SetTime ();
      if (m_gateway != Ipv4Address ())
        {
          newDhcpHeader.SetRouter (m_gateway);
        }
      packet->AddHeader (newDhcpHeader);

      if ((m_socket->SendTo (packet, 0, InetSocketAddress (Ipv4Address ("255.255.255.255"), InetSocketAddress::ConvertFrom (from).GetPort ()))) >= 0)
        {
          NS_LOG_INFO ("DHCP OFFER" << " Offered Address: " << address);
        } 
      else
        {
          NS_LOG_INFO ("Error while sending DHCP OFFER");
        }
    }
}

void DhcpServer::SendAck (Ptr<NetDevice> iDev, DhcpHeader header, InetSocketAddress from)
{
  DhcpHeader newDhcpHeader;
  Address source = header.GetChaddr ();
  uint32_t tran = header.GetTran ();
  Ptr<Packet> packet = 0;
  Ipv4Address address = header.GetReq ();

  NS_LOG_INFO ("DHCP REQUEST from: " << from.GetIpv4 () <<
               " source port: " <<  from.GetPort () <<
               " - refreshed addr: " << address);

  std::map<std::pair<Address, Ipv4Address>, uint32_t>::iterator i;
  for (i = m_leasedAddresses.begin (); i != m_leasedAddresses.end (); i++)
    {
      // update the lease time of this address
      if (i->first.second == address)
        {
          (i->second) += m_lease.GetSeconds ();
          packet = Create<Packet> ();
          newDhcpHeader.ResetOpt ();
          newDhcpHeader.SetType (DhcpHeader::DHCPACK);
          newDhcpHeader.SetChaddr (source);
          newDhcpHeader.SetYiaddr (address);
          newDhcpHeader.SetTran (tran);
          newDhcpHeader.SetTime ();
          packet->AddHeader (newDhcpHeader);
          if (from.GetIpv4 () != address)
            {
              m_socket->SendTo (packet, 0, InetSocketAddress (Ipv4Address ("255.255.255.255"), from.GetPort ()));
            }
          else
            {
              m_socket->SendTo (packet, 0, from);
            }
          break;
        }
    }
  if (i == m_leasedAddresses.end ())
    {
      packet = Create<Packet> ();
      newDhcpHeader.ResetOpt ();
      newDhcpHeader.SetType (DhcpHeader::DHCPNACK);
      newDhcpHeader.SetChaddr (source);
      newDhcpHeader.SetYiaddr (address);
      newDhcpHeader.SetTran (tran);
      newDhcpHeader.SetTime ();
      packet->AddHeader (newDhcpHeader);
      if (from.GetIpv4 () != address)
        {
          m_socket->SendTo (packet, 0, InetSocketAddress (Ipv4Address ("255.255.255.255"), from.GetPort ()));
        }
      else
        {
          m_socket->SendTo (packet, 0, from);
        }
      NS_LOG_INFO ("IP addr does not exists or released!");
    }
}

} // Namespace ns3
