/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
#include <iostream>
#include <fstream>
#include <vector>

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/names.h"
#include "ns3/string.h"
#include "ns3/random-variable-stream.h"

#include "ns3/helics.h"
#include "ns3/helics-application.h"
#include "helics/application_api/application_api.h"

#include <algorithm>
#include <sstream>
#include <string>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HelicsApplication");

NS_OBJECT_ENSURE_REGISTERED (HelicsApplication);

TypeId
HelicsApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HelicsApplication")
    .SetParent<Application> ()
    .AddConstructor<HelicsApplication> ()
    .AddAttribute ("Name", 
                   "The name of the application",
                   StringValue (),
                   MakeStringAccessor (&HelicsApplication::m_name),
                   MakeStringChecker ())
    .AddAttribute ("Sent", 
                   "The counter for outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&HelicsApplication::m_sent),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("LocalAddress", 
                   "The source Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&HelicsApplication::m_localAddress),
                   MakeAddressChecker ())
    .AddAttribute ("LocalPort", 
                   "The source port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&HelicsApplication::m_localPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("JitterMinNs",
                   "The source port of the outbound packets",
                   DoubleValue  (1000),
                   MakeDoubleAccessor (&HelicsApplication::m_jitterMinNs),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("JitterMaxNs",
                   "The source port of the outbound packets",
                   DoubleValue (100000),
                   MakeDoubleAccessor (&HelicsApplication::m_jitterMaxNs),
                   MakeDoubleChecker<double> ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&HelicsApplication::m_txTrace),
                     "ns3::Packet::TracedCallback")
    .AddAttribute ("OutFileName", 
                   "The name of the output file",
                   StringValue (),
                   MakeStringAccessor (&HelicsApplication::f_name),
                   MakeStringChecker ())
  ;
  return tid;
}

HelicsApplication::HelicsApplication ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
  
  //Setting up jitter random variable stream
  m_rand_delay_ns = CreateObject<UniformRandomVariable> ();
  m_rand_delay_ns->SetAttribute ("Min", DoubleValue  (m_jitterMinNs));
  m_rand_delay_ns->SetAttribute ("Max", DoubleValue  (m_jitterMaxNs));

  m_next_tag_id = 0;
}

HelicsApplication::~HelicsApplication()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
}

void
HelicsApplication::SetName (const std::string &name)
{
  NS_LOG_FUNCTION (this << name);
  m_name = name;
  Names::Add("helics_"+name, this);
  m_filter_id = helics_federate->registerSourceFilter ("ns3_"+name, name);
  using std::placeholders::_1;
  using std::placeholders::_2;
  std::function<void(helics::filter_id_t,helics::Time)> func;
  func = std::bind (&HelicsApplication::FilterCallback, this, _1, _2);
  helics_federate->registerFilterCallback(m_filter_id, func);
}

std::string
HelicsApplication::GetName (void) const
{
  return m_name;
}

void 
HelicsApplication::SetLocal (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_localAddress = ip;
  m_localPort = port;
}

void 
HelicsApplication::SetLocal (Ipv4Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_localAddress = Address (ip);
  m_localPort = port;
}

void 
HelicsApplication::SetLocal (Ipv6Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_localAddress = Address (ip);
  m_localPort = port;
}

void
HelicsApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
HelicsApplication::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      if (Ipv4Address::IsMatchingType(m_localAddress) == true)
        {
          m_socket->Bind (InetSocketAddress (Ipv4Address::ConvertFrom(m_localAddress), m_localPort));
        }
      else if (Ipv6Address::IsMatchingType(m_localAddress) == true)
        {
          m_socket->Bind (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_localAddress), m_localPort));
        }
      else
        {
          m_socket->Bind();
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&HelicsApplication::HandleRead, this));

  if (m_name.empty()) {
    NS_FATAL_ERROR("HelicsApplication is missing name");
  }

  if (f_name.empty())
  {
    NS_LOG_INFO("HelicsApplication is missing an output file in CSV format.");
  }
}

void 
HelicsApplication::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }
}

static uint8_t
char_to_uint8_t (char c)
{
  return uint8_t(c);
}

HelicsIdTag
HelicsApplication::NewTag ()
{
  return HelicsIdTag(m_next_tag_id++);
}

void 
HelicsApplication::FilterCallback (helics::filter_id_t id, helics::Time time)
{
  NS_LOG_FUNCTION (this << "Helics filter callback");
 
  Ptr<Packet> p;

  // Get the helics Message.
  auto m = helics_federate->getMessageToFilter (id);

  // Find the HelicsApplication for the destination.
  Ptr<HelicsApplication> to = Names::Find<HelicsApplication>("helics_"+m->dest);
  if (!to) {
    NS_FATAL_ERROR("failed HelicsApplication lookup to '" << m->dest << "'");
  }

  // Convert given Message into a Packet.
  size_t total_size = m->data.size();
  uint8_t *buffer = new uint8_t[total_size];
  uint8_t *buffer_ptr = buffer;
  std::transform (m->data.begin(), m->data.end(), buffer_ptr, char_to_uint8_t);
  p = Create<Packet> (buffer, total_size);
  NS_LOG_INFO("buffer='" << p << "'");
  delete [] buffer;

  // Create a new ID at the destination application.
  HelicsIdTag tag = to->NewTag();

  // Add tag to the packet.
  p->AddPacketTag(tag);

  // Store the Message at the destination application for later sending.
  to->m_messages.insert(std::make_pair(tag, std::move (m)));

  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_txTrace (p);
  
  int delay_ns = (int) (m_rand_delay_ns->GetValue (m_jitterMinNs,m_jitterMaxNs) + 0.5);

  if (Ipv4Address::IsMatchingType (m_localAddress))
    {
      InetSocketAddress address = to->GetLocalInet();
      if (~f_name.empty())
      {
        std::ofstream outFile(f_name.c_str(), std::ios::app);
        outFile << Simulator::Now ().GetNanoSeconds ()  << ","
                << p->GetUid () << ","
                << "s,"
                << total_size << ","
                << address.GetIpv4() << ","
                << address.GetPort();
        outFile.close();
      }
      NS_LOG_INFO ("At time '"
          << Simulator::Now ().GetNanoSeconds () + delay_ns
          << "'ns '"
          << m_name
          << "' sent "
          << total_size
          << " bytes to '"
          << to->GetName()
          << "' at address "
          << address.GetIpv4()
          << " port "
          << address.GetPort()
          << "' uid '"
          << p->GetUid () <<"'");
      //m_socket->SendTo(p, 0, address);
      //Simulator::Schedule(NanoSeconds (delay_ns), &Socket::SendTo, m_socket, buffer_ptr, p->GetSize(), 0, address); //non-virtual method
      int (Socket::*fp)(Ptr<Packet>, uint32_t, const Address&) = &Socket::SendTo;
      Simulator::Schedule(NanoSeconds (delay_ns), fp, m_socket, p, 0, address); //virtual method
    }
  else if (Ipv6Address::IsMatchingType (m_localAddress))
    {
      Inet6SocketAddress address = to->GetLocalInet6();
      if (~f_name.empty())
      {
        std::ofstream outFile(f_name.c_str(), std::ios::app);
        outFile << Simulator::Now ().GetNanoSeconds ()  << ","
                << p->GetUid () << ","
                << "s,"
                << total_size << ","
                << address.GetIpv6() << ","
                << address.GetPort();
        outFile.close();
      }
      NS_LOG_INFO ("At time '"
          << Simulator::Now ().GetNanoSeconds () + delay_ns
          << "'ns '"
          << m_name
          << "' sent "
          << total_size
          << " bytes to '"
          << to->GetName()
          << "' at address "
          << address.GetIpv6()
          << " port "
          << address.GetPort()
          << "' uid '"
          << p->GetUid () <<"'");
      //m_socket->SendTo(p, 0, address);
      //Simulator::Schedule(NanoSeconds (delay_ns), &Socket::SendTo, m_socket, buffer_ptr, p->GetSize(), 0, address); //non-virtual method
      int (Socket::*fp)(Ptr<Packet>, uint32_t, const Address&) = &Socket::SendTo;
      Simulator::Schedule(NanoSeconds (delay_ns), fp, m_socket, p, 0, address); //virtual method
    }
  ++m_sent;
}

InetSocketAddress HelicsApplication::GetLocalInet (void) const
{
  return InetSocketAddress(Ipv4Address::ConvertFrom(m_localAddress), m_localPort);
}

Inet6SocketAddress HelicsApplication::GetLocalInet6 (void) const
{
  return Inet6SocketAddress(Ipv6Address::ConvertFrom(m_localAddress), m_localPort);
}

void
HelicsApplication::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      HelicsIdTag tag;
      if (!packet->PeekPacketTag(tag)) {
          NS_LOG_INFO ("Reading packet without HelicsIdTag, skipping");
          continue;
      }

      // For printing.
      uint32_t size = packet->GetSize();
      std::ostringstream odata;
      packet->CopyData (&odata, size);
      std::string sdata = odata.str();

      // Locate our Message
      std::map<uint32_t,std::unique_ptr<helics::Message> >::iterator item;
      item = m_messages.find(tag);
      if (m_messages.end() == item) {
          NS_LOG_INFO("Reading packet but HelicsIdTag not found: " << tag);
          continue;
      }

      // Sanity check that it's the same size.
      if (item->second->data.size() != size) {
          NS_LOG_INFO ("Reading packet but size differs from Message: "
                  << item->second->data.size() << " != " << size);
      }

      // Send on the filtered Message
      helics_federate->sendMessage (helics_endpoint, std::move (item->second));

      if (InetSocketAddress::IsMatchingType (from))
        {
          if (~f_name.empty())
          {
            std::ofstream outFile(f_name.c_str(), std::ios::app);
            outFile << Simulator::Now ().GetNanoSeconds ()  << ","
                    << packet->GetUid () << ","
                    << "r,"
                    << size << ","
                    << InetSocketAddress::ConvertFrom (from).GetIpv4 () << ","
                    << InetSocketAddress::ConvertFrom (from).GetPort () << ","
                    << sdata << std::endl;
            outFile.close();
          }
          NS_LOG_INFO ("At time '"
                  << Simulator::Now ().GetNanoSeconds ()
                  << "'ns '"
                  << m_name
                  << "' received "
                  << size
                  << " bytes at address "
                  << InetSocketAddress::ConvertFrom (from).GetIpv4 ()
                  << " port "
                  << InetSocketAddress::ConvertFrom (from).GetPort ()
                  << "' sdata '"
                  << sdata
                  << "' uid '"
                  << packet->GetUid () <<"'");
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          if (~f_name.empty())
          {
            std::ofstream outFile(f_name.c_str(), std::ios::app);
            outFile << Simulator::Now ().GetNanoSeconds ()  << ","
                    << packet->GetUid () << ","
                    << "r,"
                    << size << ","
                    << Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << ","
                    << Inet6SocketAddress::ConvertFrom (from).GetPort () << ","
                    << sdata << std::endl;
            outFile.close();
          }
          NS_LOG_INFO ("At time '"
                  << Simulator::Now ().GetNanoSeconds ()
                  << "'ns '"
                  << m_name
                  << "' received "
                  << size
                  << " bytes at address "
                  << Inet6SocketAddress::ConvertFrom (from).GetIpv6 ()
                  << " port "
                  << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                  << "' sdata '"
                  << sdata
                  << "' uid '"
                  << packet->GetUid () <<"'");
        }
    }
}

} // Namespace ns3
