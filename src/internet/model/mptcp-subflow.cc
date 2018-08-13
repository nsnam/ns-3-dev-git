/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Sussex
 * Copyright (c) 2015 Université Pierre et Marie Curie (UPMC)
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
 * Author:  Kashif Nadeem <kshfnadeem@gmail.com>
 *          Matthieu Coudron <matthieu.coudron@lip6.fr>
 *          Morteza Kheirkhah <m.kheirkhah@sussex.ac.uk>
 */
#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
  if (m_node) { std::clog << Simulator::Now ().GetSeconds () << " [node " << m_node->GetId () << ": ] "; }

#include <iostream>
#include <cmath>
#include "ns3/mptcp-mapping.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/mptcp-subflow.h"
#include "ns3/mptcp-socket-base.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-end-point.h"
#include "ns3/node.h"
#include "ns3/ptr.h"
#include "ns3/tcp-option-mptcp.h"
#include "ns3/ipv4-address.h"
#include "ns3/trace-helper.h"
#include <algorithm>
#include <openssl/sha.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("MpTcpSubflow");

NS_OBJECT_ENSURE_REGISTERED(MpTcpSubflow);

static inline
SequenceNumber32 SEQ64TO32(SequenceNumber64 seq)
{
  return SequenceNumber32( seq.GetValue());
}

static inline
SequenceNumber64 SEQ32TO64(SequenceNumber32 seq)
{
  return SequenceNumber64( seq.GetValue());
}

TypeId
MpTcpSubflow::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::MpTcpSubflow")
      .SetParent<TcpSocketBase>()
      .SetGroupName ("Internet")
      .AddConstructor<MpTcpSubflow>()
    ;
  return tid;
}

//! wrapper function
static inline
MpTcpMapping
GetMapping(const Ptr<const TcpOptionMpTcpDSS> dss)
{
  MpTcpMapping mapping;
  uint64_t dsn;
  uint32_t ssn;
  uint16_t length;
  dss->GetMapping (dsn, ssn, length);
  mapping.SetHeadDSN( SequenceNumber64(dsn));
  mapping.SetMappingSize(length);
  mapping.MapToSSN( SequenceNumber32(ssn));
  return mapping;
}

TypeId
MpTcpSubflow::GetInstanceTypeId(void) const
{
  return MpTcpSubflow::GetTypeId();
}

void
MpTcpSubflow::SetMeta(Ptr<MpTcpSocketBase> metaSocket)
{
  NS_ASSERT(metaSocket);
  NS_LOG_FUNCTION(this);
  m_metaSocket = metaSocket;
}

void
MpTcpSubflow::DumpInfo() const
{
  NS_LOG_LOGIC ("MpTcpSubflow " << this << " SendPendingData" <<
          " rxwin " << m_rWnd <<
          " nextTxSeq " << m_tcb->m_nextTxSequence <<
          " highestRxAck " << m_txBuffer->HeadSequence ()<< 
          " pd->SFS " << m_txBuffer->SizeFromSequence (m_tcb->m_nextTxSequence)
          );
}

void
MpTcpSubflow::CancelAllTimers()
{
  NS_LOG_FUNCTION(this);
  m_retxEvent.Cancel();
  m_lastAckEvent.Cancel();
  m_timewaitEvent.Cancel();
  NS_LOG_LOGIC( "CancelAllTimers");
}

int
MpTcpSubflow::DoConnect()
{
  NS_LOG_FUNCTION (this);
  return TcpSocketBase::DoConnect();

}

/* Inherit from Socket class: Kill this socket and signal the peer (if any) */
int
MpTcpSubflow::Close(void)
{
  NS_LOG_FUNCTION (this);
  return TcpSocketBase::Close();
}

/* If copied from a legacy socket, then it's a master socket */
MpTcpSubflow::MpTcpSubflow(const TcpSocketBase& sock)
  : TcpSocketBase(sock),
    m_dssFlags(0),
    m_masterSocket(true)
{
  NS_LOG_FUNCTION (this << &sock);
  NS_LOG_LOGIC ("Copying from TcpSocketBase/check2. endPoint=" << sock.m_endPoint);
  // We need to update the endpoint callbnacks so that packets come to this socket
  // instead of the abstract meta
  // this is necessary for the client socket
  NS_LOG_UNCOND("Cb=" << m_sendCb.IsNull () << " endPoint=" << m_endPoint);
  m_endPoint = (sock.m_endPoint);
  m_endPoint6 = (sock.m_endPoint6);
  SetupCallback();
}

// Does this constructor even make sense ? no ? to remove ?
MpTcpSubflow::MpTcpSubflow(const MpTcpSubflow& sock)
  : TcpSocketBase(sock),
    m_dssFlags(0),
    m_masterSocket(sock.m_masterSocket),
    m_localNonce(sock.m_localNonce)
{
  NS_LOG_FUNCTION (this << &sock);
  NS_LOG_LOGIC ("Invoked the copy constructor/check2");
}

MpTcpSubflow::MpTcpSubflow()
  : TcpSocketBase(),
    m_metaSocket(0),
    m_masterSocket(false),
    m_backupSubflow(false),
    m_localNonce(0)
{
  NS_LOG_FUNCTION(this);
}

MpTcpSubflow::~MpTcpSubflow()
{
  NS_LOG_FUNCTION(this);
}

void
MpTcpSubflow::CloseAndNotify(void)
{
  NS_LOG_FUNCTION_NOARGS();
  TcpSocketBase::CloseAndNotify();
  GetMeta()->OnSubflowClosed( this, false );
}

/* Mapping should already exist when sending the packet */
int
MpTcpSubflow::Send(Ptr<Packet> p, uint32_t flags)
{
  NS_LOG_FUNCTION(this);
  int ret = TcpSocketBase::Send(p, flags);
  if(ret > 0)
   {
     MpTcpMapping temp;
     SequenceNumber32 ssnHead = m_txBuffer->TailSequence() - p->GetSize();
     NS_ASSERT(m_TxMappings.GetMappingForSSN(ssnHead, temp));
   }
  return ret;
}

void
MpTcpSubflow::SendEmptyPacket(uint8_t flags)
{
  NS_LOG_FUNCTION_NOARGS();
  TcpSocketBase::SendEmptyPacket(flags);
}

bool
MpTcpSubflow::AddLooseMapping(SequenceNumber64 dsnHead, uint16_t length)
{
  NS_LOG_LOGIC("Adding mapping with dsn=" << dsnHead << " len=" << length);
  MpTcpMapping mapping;

  mapping.MapToSSN(FirstUnmappedSSN());
  mapping.SetMappingSize(length);
  mapping.SetHeadDSN(dsnHead);
  bool ok = m_TxMappings.AddMapping( mapping  );
  NS_ASSERT_MSG( ok, "Can't add mapping: 2 mappings overlap");
  return ok;
}

SequenceNumber32
MpTcpSubflow::FirstUnmappedSSN()
{
  NS_LOG_FUNCTION(this);
  SequenceNumber32 ssn;
  if(!m_TxMappings.FirstUnmappedSSN(ssn))
    {
      ssn = m_txBuffer->TailSequence();
    }
  return ssn;
}

void
MpTcpSubflow::GetMappedButMissingData(
                std::set< MpTcpMapping >& missing
                )
{
  NS_LOG_FUNCTION(this);
  SequenceNumber32 startingSsn = m_txBuffer->TailSequence();
  m_TxMappings.GetMappingsStartingFromSSN(startingSsn, missing);
}

void
MpTcpSubflow::SendPacket(TcpHeader header, Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << header <<  p);
  Ptr<const TcpOptionMpTcpJoin> join;   
  if (p->GetSize() && !IsInfiniteMappingEnabled() && !GetTcpOption(header, join))
    {
      //... we must decide to send a mapping or not
      // For now we always append the mapping but we could have mappings spanning over several packets.
      // and thus not add the mapping for several packets
      ///============================

      SequenceNumber32 ssnHead = header.GetSequenceNumber();
      MpTcpMapping mapping;

      bool result = m_TxMappings.GetMappingForSSN(ssnHead, mapping);
      if (!result)
        {
          m_TxMappings.Dump();
          NS_FATAL_ERROR("Could not find mapping associated to ssn");
        }
      NS_ASSERT_MSG(mapping.TailSSN() >= ssnHead +p->GetSize() -1, "mapping should cover the whole packet" );
      AppendDSSMapping(mapping);
   }
  // we append hte ack everytime
  TcpSocketBase::SendPacket(header, p);
  m_dssFlags = 0; // reset for next packet
}

uint32_t
MpTcpSubflow::SendDataPacket(SequenceNumber32 ssnHead, uint32_t maxSize, bool withAck)
{
  NS_LOG_FUNCTION(this << "Sending packet starting at SSN [" << ssnHead.GetValue() << "] with len=" << maxSize<< withAck);
  MpTcpMapping mapping;
  bool result = m_TxMappings.GetMappingForSSN(ssnHead, mapping);
  if (!result)
    {
      m_TxMappings.Dump();
      NS_FATAL_ERROR("Could not find mapping associated to ssn");
    }
  AppendDSSMapping(mapping);
  // Here we set the maxsize to the size of the mapping
  return TcpSocketBase::SendDataPacket(ssnHead, std::min( (int)maxSize,mapping.TailSSN()-ssnHead+1), withAck);
}

bool
MpTcpSubflow::IsInfiniteMappingEnabled() const
{
  return GetMeta()->IsInfiniteMappingEnabled();
}

void
MpTcpSubflow::Retransmit(void)
{
  NS_LOG_FUNCTION (this);
}

void
MpTcpSubflow::DoRetransmit()
{
  NS_LOG_FUNCTION(this);

  GetMeta()->OnSubflowRetransmit(this);
  TcpSocketBase::DoRetransmit();      
}

/* Received a packet upon LISTEN state. */
void
MpTcpSubflow::ProcessListen(Ptr<Packet> packet, const TcpHeader& tcpHeader, const Address& fromAddress, const Address& toAddress)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  NS_FATAL_ERROR("This function should never be called, shoud it ?!");
  // Extract the flags. PSH and URG are not honoured.
  uint8_t tcpflags = tcpHeader.GetFlags() & ~(TcpHeader::PSH | TcpHeader::URG);

  // Fork a socket if received a SYN. Do nothing otherwise.
  // C.f.: the LISTEN part in tcp_v4_do_rcv() in tcp_ipv4.c in Linux kernel
  if (tcpflags != TcpHeader::SYN)
    {
      return;
    }
  if (!NotifyConnectionRequest(fromAddress))
    {
      return;
    }
  NS_LOG_LOGIC("Updating receive window" << tcpHeader.GetWindowSize());

  // Clone the socket, simulate fork
  Ptr<MpTcpSubflow> newSock = DynamicCast<MpTcpSubflow>(Fork());
  NS_LOG_LOGIC ("Cloned a TcpSocketBase " << newSock);
  Simulator::ScheduleNow(
      &MpTcpSubflow::CompleteFork,
      newSock,
      packet,
      tcpHeader,
      fromAddress,
      toAddress
      );
}

Ptr<MpTcpSocketBase>
MpTcpSubflow::GetMeta() const
{
  NS_ASSERT(m_metaSocket);
  return m_metaSocket;
}

/* It is also encouraged to
   reduce the timeouts (Maximum Segment Life) on subflows at end hosts.
   Move TCP to Time_Wait state and schedule a transition to Closed state */
void
MpTcpSubflow::TimeWait()
{
  NS_LOG_INFO (TcpStateName[m_state] << " -> TIME_WAIT");
  m_state = TIME_WAIT;
  CancelAllTimers();
  // Move from TIME_WAIT to CLOSED after 2*MSL. Max segment lifetime is 2 min
  // according to RFC793, p.28
  m_timewaitEvent = Simulator::Schedule(Seconds( m_msl), &MpTcpSubflow::CloseAndNotify, this);
}

void
MpTcpSubflow::AddMpTcpOptionDSS(TcpHeader& header)
{
  NS_LOG_FUNCTION(this);
  Ptr<TcpOptionMpTcpDSS> dss = Create<TcpOptionMpTcpDSS>();
  const bool sendDataFin = m_dssFlags &  TcpOptionMpTcpDSS::DataFin;
  const bool sendDataAck = m_dssFlags & TcpOptionMpTcpDSS::DataAckPresent;

  if(sendDataAck)
  {
    uint32_t dack = GetMeta()->GetRxBuffer()->NextRxSequence().GetValue();
    dss->SetDataAck( dack );
  }

  // If no mapping set but datafin set , we have to create the mapping from scratch
  if (sendDataFin)
   {
     m_dssMapping.MapToSSN(SequenceNumber32(0));
     m_dssMapping.SetHeadDSN(SEQ32TO64(GetMeta()->m_txBuffer->TailSequence() ));
     m_dssMapping.SetMappingSize(1);
     m_dssFlags |= TcpOptionMpTcpDSS::DSNMappingPresent;
   }

  // if there is a mapping to send
  if(m_dssFlags & TcpOptionMpTcpDSS::DSNMappingPresent)
  {
    dss->SetMapping(m_dssMapping.HeadDSN().GetValue(), m_dssMapping.HeadSSN().GetValue(),
                           m_dssMapping.GetLength(), sendDataFin);
   }
  header.AppendOption(dss);
}

void
MpTcpSubflow::AddMpTcpOptions (TcpHeader& header)
{
  NS_LOG_FUNCTION(this);

  Ptr<const TcpOptionMpTcpJoin> join;
  if ((header.GetFlags () & TcpHeader::SYN))
    {
      AddOptionMpTcp3WHS(header);
    }
    // as long as we've not received an ack from the peer we
    // send an MP_CAPABLE with both keys
  else if(!GetMeta()->FullyEstablished())
    {
      AddOptionMpTcp3WHS(header);
    }
  /// Constructs DSS if necessary
  /////////////////////////////////////////

  if (m_dssFlags && !GetTcpOption(header, join) )
    {
      AddMpTcpOptionDSS(header);
    }
}

void
MpTcpSubflow::ProcessClosing(Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  return TcpSocketBase::ProcessClosing(packet,tcpHeader);
}

/* Received a packet upon CLOSE_WAIT, FIN_WAIT_1, or FIN_WAIT_2 states */
void
MpTcpSubflow::ProcessWait(Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);
  TcpSocketBase::ProcessWait(packet,tcpHeader);
}

/* Deallocate the end point and cancel all the timers */
void
MpTcpSubflow::DeallocateEndPoint(void)
{
  NS_LOG_FUNCTION(this);
  TcpSocketBase::DeallocateEndPoint();
}

void
MpTcpSubflow::CompleteFork(Ptr<Packet> p, const TcpHeader& h, const Address& fromAddress, const Address& toAddress)
{
  NS_LOG_INFO( this << "Completing fork of MPTCP subflow");

  if (IsMaster())
    {
      GetMeta()->GenerateUniqueMpTcpKey();
    }
  TcpSocketBase::CompleteFork(p, h, fromAddress, toAddress);

  if (IsMaster())
    {
      NS_LOG_LOGIC("Setting meta endpoint to " << m_endPoint
                   << " (old endpoint=" << GetMeta()->m_endPoint << " )");
      GetMeta()->m_endPoint = m_endPoint;
    }
   NS_LOG_LOGIC("Setting subflow endpoint to " << m_endPoint); 
}

/* Clean up after Bind. Set up callback functions in the end-point.
   This function is added to call subflow endpoint to handle receive packet. */
int
MpTcpSubflow::SetupCallback (void)
{
  NS_LOG_FUNCTION(this);
  return TcpSocketBase::SetupCallback();
}

void
MpTcpSubflow::ForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port,
                          Ptr<Ipv4Interface> incomingInterface)
{
  NS_LOG_LOGIC ("Socket " << this << " forward up " <<
                m_endPoint->GetPeerAddress () <<
                ":" << m_endPoint->GetPeerPort () <<
                " to " << m_endPoint->GetLocalAddress () <<
                ":" << m_endPoint->GetLocalPort ());
  TcpSocketBase::ForwardUp(packet, header, port, incomingInterface);
}

/* Apparently this function is never called for now */
void
MpTcpSubflow::ConnectionSucceeded(void)
{
  NS_LOG_LOGIC(this << "Connection succeeded");
  m_connected = true;
}

/* Received a packet upon SYN_SENT */
void
MpTcpSubflow::ProcessSynSent(Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  NS_ASSERT(m_state == SYN_SENT);
  TcpSocketBase::ProcessSynSent(packet, tcpHeader);
}

int
MpTcpSubflow::ProcessOptionMpTcpCapable(const Ptr<const TcpOptionMpTcpMain> option)
{
  NS_LOG_LOGIC(this << option);
  NS_ASSERT_MSG(IsMaster(), "You can receive MP_CAPABLE only on the master subflow");

  // Expect an MP_CAPABLE option
  Ptr<const TcpOptionMpTcpCapable> mpcRcvd = DynamicCast<const TcpOptionMpTcpCapable>(option);
  NS_ASSERT_MSG(mpcRcvd, "There must be a MP_CAPABLE option");
  GetMeta()->SetPeerKey( mpcRcvd->GetSenderKey() );
  return 0;
}

int
MpTcpSubflow::ProcessOptionMpTcpJoin(const Ptr<const TcpOptionMpTcpMain> option)
{
  NS_LOG_FUNCTION(this << option);

  Ptr<const TcpOptionMpTcpJoin> join = DynamicCast<const TcpOptionMpTcpJoin>(option);
  NS_ASSERT_MSG( join, "There must be an MP_JOIN option in the SYN Packet" );
  NS_ASSERT_MSG( join && join->GetMode() == TcpOptionMpTcpJoin::SynAck, "the MPTCP join option received is not of the expected 1 out of 3 MP_JOIN types." );
  return 0;
}

//This functions process MP_ADD_ADDR mptcp options
int
MpTcpSubflow::ProcessOptionMpTcpAddAddress(const Ptr<const TcpOptionMpTcpAddAddress> addaddr) 
{
  NS_LOG_FUNCTION (this << addaddr << " MP_ADD_ADDR ");
  InetSocketAddress  address= InetSocketAddress::ConvertFrom (addaddr->GetAddress ());

  Ipv4Address destinationIp = address.GetIpv4 ();
  uint16_t    destinationPort = address.GetPort ();  
  GetMeta()->RemoteAddressInfo.push_back(std::make_pair(destinationIp, destinationPort));
  return 0;
}

int
MpTcpSubflow::ProcessOptionMpTcp (const Ptr<const TcpOption> option)
{
  //! adds the header
  NS_LOG_FUNCTION(option);
  Ptr<const TcpOptionMpTcpMain> main = DynamicCast<const TcpOptionMpTcpMain>(option);
  switch(main->GetSubType())
     {
       case TcpOptionMpTcpMain::MP_CAPABLE:
            return ProcessOptionMpTcpCapable(main);
       case TcpOptionMpTcpMain::MP_JOIN:
            return ProcessOptionMpTcpJoin(main);
       case TcpOptionMpTcpMain::MP_DSS:
            {
              Ptr<const TcpOptionMpTcpDSS> dss = DynamicCast<const TcpOptionMpTcpDSS>(option);
              NS_ASSERT(dss);
              // Update later on
              ProcessOptionMpTcpDSSEstablished(dss);
            }
            break;
       case TcpOptionMpTcpMain::MP_ADD_ADDR: 
            {
              Ptr<const TcpOptionMpTcpAddAddress> addaddr = DynamicCast<const TcpOptionMpTcpAddAddress>(option);
              NS_ASSERT(addaddr);
              ProcessOptionMpTcpAddAddress(addaddr);   
            }
            break;   
       case TcpOptionMpTcpMain::MP_FASTCLOSE:
       case TcpOptionMpTcpMain::MP_FAIL:
       default:
            NS_FATAL_ERROR("Unsupported yet");
            break;
      };
  return 0;
}

void
MpTcpSubflow::AddOptionMpTcp3WHS(TcpHeader& hdr) const
{
  NS_LOG_FUNCTION(this << hdr << hdr.FlagsToString(hdr.GetFlags()));

  if ( IsMaster() )
    {
     //! Use an MP_CAPABLE option
     Ptr<TcpOptionMpTcpCapable> mpc =  CreateObject<TcpOptionMpTcpCapable>();
     switch(hdr.GetFlags())
     {
       case TcpHeader::SYN:
       case (TcpHeader::SYN | TcpHeader::ACK):
         mpc->SetSenderKey( GetMeta()->GetLocalKey() );
         break;
       case TcpHeader::ACK:
         mpc->SetSenderKey( GetMeta()->GetLocalKey() );
         mpc->SetPeerKey( GetMeta()->GetPeerKey() );
         break;
       default:
         NS_FATAL_ERROR("Should never happen");
         break;
     };
  NS_LOG_INFO("Appended option" << mpc);
  hdr.AppendOption( mpc );
    }
  else
   { 
     Ptr<TcpOptionMpTcpJoin> join =  CreateObject<TcpOptionMpTcpJoin>();
     switch(hdr.GetFlags())
     {
       case TcpHeader::SYN:
         {
           join->SetMode(TcpOptionMpTcpJoin::Syn);
           join->SetPeerToken(GetMeta()->GetPeerToken());
           join->SetNonce(0);
         }
         break;
       case TcpHeader::ACK:
         {
           uint8_t hmac[20];

           join->SetMode(TcpOptionMpTcpJoin::Ack);
           join->SetHmac( hmac );
         }
         break;
       case (TcpHeader::SYN | TcpHeader::ACK):
         {
           join->SetMode(TcpOptionMpTcpJoin::SynAck);
           static uint8_t id = 0;
           NS_LOG_WARN("IDs are incremental, there is no real logic behind it yet");
           join->SetAddressId( id++ );
           join->SetTruncatedHmac(424242); // who cares 
           join->SetNonce(4242); //! truly random :)
         }
         break;
       default:
         NS_FATAL_ERROR("Should never happen");
         break;
    }
     NS_LOG_INFO("Appended option" << join);
     hdr.AppendOption( join );
  }
}

// This function to pass peer token to tcpsocketbase
uint32_t
MpTcpSubflow::PeerToken()
{
  uint32_t dummy = GetMeta()->GetPeerToken();
  return dummy;
}

int
MpTcpSubflow::Listen(void)
{
  NS_FATAL_ERROR("This should never be called. The meta will make the subflow pass from LISTEN to ESTABLISHED.");
}

void
MpTcpSubflow::NotifySend (uint32_t spaceAvailable)
{
  GetMeta()->NotifySend(spaceAvailable);
}

/* if one looks at the linux kernel, tcp_synack_options */
void
MpTcpSubflow::ProcessSynRcvd(Ptr<Packet> packet, const TcpHeader& tcpHeader, const Address& fromAddress,
    const Address& toAddress)
{
  NS_LOG_FUNCTION (this << tcpHeader);
  TcpSocketBase::ProcessSynRcvd(packet, tcpHeader, fromAddress, toAddress);

  //receiver node advertises addresses to the peer after complete fork 
  GetMeta()->AdvertiseAddresses();
}

uint32_t
MpTcpSubflow::SendPendingData(bool withAck)
{
  NS_LOG_FUNCTION(this);
  return TcpSocketBase::SendPendingData(withAck);
}

bool
MpTcpSubflow::IsMaster() const
{
  return m_masterSocket;
}

bool
MpTcpSubflow::BackupSubflow() const
{
  return m_backupSubflow;
}

/* should be able to advertise several in one packet if enough space
   It is possible
   http://tools.ietf.org/html/rfc6824#section-3.4.1
   A host can send an ADD_ADDR message with an already assigned Address
   ID, but the Address MUST be the same as previously assigned to this
   Address ID, and the Port MUST be different from one already in use
   for this Address ID.  If these conditions are not met, the receiver
   SHOULD silently ignore the ADD_ADDR.  A host wishing to replace an
   existing Address ID MUST first remove the existing one
   (Section 3.4.2).

   A host that receives an ADD_ADDR but finds a conn
   ection set up to
   that IP address and port number is unsuccessful SHOULD NOT perform
   further connection attempts to this address/port combination for this
   connection.  A sender that wants to trigger a new incoming connection
   attempt on a previously advertised address/port combination can
   therefore refresh ADD_ADDR information by sending the option again. */
void
MpTcpSubflow::AdvertiseAddress(Ipv4Address addr, uint16_t port)
{
  NS_LOG_FUNCTION("Started advertising address");
}

bool
MpTcpSubflow::StopAdvertisingAddress(Ipv4Address address)
{
  return true;
}

void
MpTcpSubflow::ReTxTimeout()
{
  NS_LOG_LOGIC("MpTcpSubflow ReTxTimeout expired !");
  TcpSocketBase::ReTxTimeout();
}

bool
MpTcpSubflow::UpdateWindowSize(const TcpHeader& header)
{
  bool updated = TcpSocketBase::UpdateWindowSize(header);
  if(updated)
   {
     GetMeta()->UpdateWindowSize(header);
    }
  return updated;
}

uint32_t
MpTcpSubflow::GetTxAvailable() const
{
  return TcpSocketBase::GetTxAvailable();
}

/* Receipt of new packet, put into Rx buffer
   SlowStart and fast recovery remains untouched in MPTCP.
   The reaction should be different depending on if we handle NR-SACK or not */
void
MpTcpSubflow::NewAck(SequenceNumber32 const& ack, bool resetRTO)
{
  NS_LOG_FUNCTION (this << resetRTO << ack);
  TcpSocketBase::NewAck(ack, resetRTO);
}

/* this is private */
Ptr<Packet>
MpTcpSubflow::ExtractAtMostOneMapping(uint32_t maxSize, bool only_full_mapping, SequenceNumber64& headDSN)
{
  MpTcpMapping mapping;
  Ptr<Packet> p = Create<Packet>();
  uint32_t rxAvailable = GetRxAvailable();
  if(rxAvailable == 0)
  {
    NS_LOG_LOGIC("Nothing to extract");
    return p;
  }
  else
  {
    NS_LOG_LOGIC(rxAvailable  << " Rx available");
  }

  // as in linux, we extract in order
  SequenceNumber32 headSSN = m_rxBuffer->HeadSequence();
  if(!m_RxMappings.GetMappingForSSN(headSSN, mapping))
   {
      m_RxMappings.Dump();
      NS_FATAL_ERROR("Could not associate a mapping to ssn [" << headSSN << "]. Should be impossible");
   }
  headDSN = mapping.HeadDSN();

  if(only_full_mapping) {

    if(mapping.GetLength() > maxSize)
    {
      NS_LOG_DEBUG("Not enough space available to extract the full mapping");
      return p;
    }
    if(m_rxBuffer->Available() < mapping.GetLength())
    {
      NS_LOG_DEBUG("Mapping not fully received yet");
      return p;
    }
  }

  // Extract at most one mapping
  maxSize = std::min(maxSize, (uint32_t)mapping.GetLength());
  NS_LOG_DEBUG("Extracting at most " << maxSize << " bytes ");
  p = m_rxBuffer->Extract( maxSize );
  SequenceNumber32 extractedTail = headSSN + p->GetSize() - 1;
  NS_ASSERT_MSG( extractedTail <= mapping.TailSSN(), "Can not extract more than the size of the mapping");

  if(extractedTail < mapping.TailSSN() )
  {
    NS_ASSERT_MSG(!only_full_mapping, "The only extracted size possible should be the one of the mapping");
    // only if data extracted covers full mapping we can remove the mapping
  }
  else
  {
    m_RxMappings.DiscardMapping(mapping);
  }
  return p;
}

void
MpTcpSubflow::AppendDSSMapping(const MpTcpMapping& mapping)
{
  NS_LOG_FUNCTION(this << mapping);
  m_dssFlags |= TcpOptionMpTcpDSS::DSNMappingPresent;
  m_dssMapping = mapping;
}

void
MpTcpSubflow::AppendDSSAck()
{
  NS_LOG_FUNCTION(this);
  m_dssFlags |= TcpOptionMpTcpDSS::DataAckPresent;
}

void
MpTcpSubflow::AppendDSSFin()
{
  NS_LOG_FUNCTION(this);
  m_dssFlags |= TcpOptionMpTcpDSS::DataFin;
}

void
MpTcpSubflow::ReceivedData(Ptr<Packet> p, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);
  MpTcpMapping mapping;
  bool sendAck = false;

  // OutOfRange
  // If cannot find an adequate mapping, then it should [check RFC]
  if(!m_RxMappings.GetMappingForSSN(tcpHeader.GetSequenceNumber(), mapping) )
   {
     m_RxMappings.Dump();
     NS_FATAL_ERROR("Could not find mapping associated ");
     return;
   }
  // Put into Rx buffer
  SequenceNumber32 expectedSSN = m_rxBuffer->NextRxSequence();
  if (!m_rxBuffer->Add(p, tcpHeader.GetSequenceNumber()))
    { // Insert failed: No data or RX buffer full
      NS_LOG_WARN("Insert failed, No data (" << p->GetSize() << ") ?");

      m_rxBuffer->Dump();
      AppendDSSAck();
      SendEmptyPacket(TcpHeader::ACK);
      return;
    }

  // Size() = Get the actual buffer occupancy
  if (m_rxBuffer->Size() > m_rxBuffer->Available() /* Out of order packets exist in buffer */
    || m_rxBuffer->NextRxSequence() > expectedSSN + p->GetSize() /* or we filled a gap */
    )
    { // A gap exists in the buffer, or we filled a gap: Always ACK
      sendAck = true;
    }
  else
    { 
      sendAck = true;
    }
  // Notify app to receive if necessary
  if (expectedSSN < m_rxBuffer->NextRxSequence())
    { // NextRxSeq advanced, we have something to send to the app
      if (!m_shutdownRecv)
        {
          GetMeta()->OnSubflowRecv( this );
          sendAck = true;
        }
      // Handle exceptions
      if (m_closeNotified)
        {
          NS_LOG_WARN ("Why TCP " << this << " got data after close notification?");
        }
      // If we received FIN before and now completed all "holes" in rx buffer,
      // invoke peer close procedure
      if (m_rxBuffer->Finished() && (tcpHeader.GetFlags() & TcpHeader::FIN) == 0)
        {
          DoPeerClose();
        }
    }
  // For now we always sent an ack
  // should be always true hack to allow compilation
  if (sendAck)
    {
      SendEmptyPacket(TcpHeader::ACK);
    }
}

uint32_t
MpTcpSubflow::UnAckDataCount() const
{
  NS_LOG_FUNCTION (this);
  return TcpSocketBase::UnAckDataCount();
}

uint32_t
MpTcpSubflow::BytesInFlight() const
{
  NS_LOG_FUNCTION (this);
  return TcpSocketBase::BytesInFlight();
}

uint32_t
MpTcpSubflow::AvailableWindow() const
{
  NS_LOG_FUNCTION (this);
  return TcpSocketBase::AvailableWindow();
}

/* this should be ok */
uint32_t
MpTcpSubflow::Window (void) const
{
  NS_LOG_FUNCTION (this);
  return TcpSocketBase::Window();
}

uint16_t
MpTcpSubflow::AdvertisedWindowSize(bool scale) const
{
  NS_LOG_DEBUG(this<<scale);
  return GetMeta()->AdvertisedWindowSize();
}

void
MpTcpSubflow::ClosingOnEmpty(TcpHeader& header)
{
  NS_LOG_FUNCTION(this << "mattator");
  header.SetFlags( header.GetFlags() | TcpHeader::FIN);
  if (m_state == ESTABLISHED)
    { // On active close: I am the first one to send FIN
      NS_LOG_INFO ("ESTABLISHED -> FIN_WAIT_1");
      m_state = FIN_WAIT_1;
    }
  else if (m_state == CLOSE_WAIT)
    {
      // On passive close: Peer sent me FIN already
      NS_LOG_INFO ("CLOSE_WAIT -> LAST_ACK");
      m_state = LAST_ACK;
    }
  GetMeta()->OnSubflowClosing(this);
}

int
MpTcpSubflow::ProcessOptionMpTcpDSSEstablished(const Ptr<const TcpOptionMpTcpDSS> dss)
{
  NS_LOG_FUNCTION (this << dss << " from subflow ");

  if(!GetMeta()->FullyEstablished() )
  {
    NS_LOG_LOGIC("First DSS received !");
    GetMeta()->BecomeFullyEstablished();
  }

  //! datafin case handled at the start of the function
  if( (dss->GetFlags() & TcpOptionMpTcpDSS::DSNMappingPresent) && !dss->DataFinMappingOnly() )
  {
    MpTcpMapping m;
    //Get mapping n'est utilisé qu'une fois, copier le code ici
    m = GetMapping(dss);
    // Add peer mapping
    bool ok = m_RxMappings.AddMapping( m );
    if(!ok)
      {
        NS_LOG_WARN("Could not insert mapping: already received ?");
        NS_LOG_UNCOND("Dumping Rx mappings...");
        m_RxMappings.Dump();
      }
  }
  if ( dss->GetFlags() & TcpOptionMpTcpDSS::DataFin)
  {
    NS_LOG_LOGIC("DFIN detected " << dss->GetDataFinDSN());
    GetMeta()->PeerClose( SequenceNumber32(dss->GetDataFinDSN()), this);
  }

  if( dss->GetFlags() & TcpOptionMpTcpDSS::DataAckPresent)
  {
    GetMeta()->ReceivedAck( SequenceNumber32(dss->GetDataAck()), this, false);
  }
  return 0;
}

/* Upon ack receival we need to act depending on if it's new or not
   if it's new it may allow us to discard a mapping
   otherwise notify meta of duplicate
   this is called */
void
MpTcpSubflow::ReceivedAck(Ptr<Packet> p, const TcpHeader& header)
{
  NS_LOG_FUNCTION (this << header);

  // if packet size > 0 then it will call ReceivedData
  TcpSocketBase::ReceivedAck(p, header );
  // By default we always append a DACK
  // We should consider more advanced schemes
  AppendDSSAck();
}

} // end of ns3
