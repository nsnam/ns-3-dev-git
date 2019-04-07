/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 NITK Surathkal
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
 * Authors: Chakradhar M <chakrdharmalagundla@gmail.com> Sri Charan M <sreecharanmallu04@gmail.com> Umesh sai<saiumesh333@gmail.com>
 */

 #include "ns3/ipv4.h"
 #include "ns3/ipv6.h"
 #include "ns3/ipv4-interface-address.h"
 #include "ns3/ipv4-route.h"
 #include "ns3/ipv6-route.h"
 #include "ns3/ipv4-routing-protocol.h"
 #include "ns3/ipv6-routing-protocol.h"
 #include "../model/ipv4-end-point.h"
 #include "../model/ipv6-end-point.h"
 #include "tcp-general-test.h"
 #include "ns3/node.h"
 #include "ns3/log.h"
 #include "tcp-error-model.h"
 #include "ns3/tcp-l4-protocol.h"
 #include "ns3/tcp-tx-buffer.h"
 #include "ns3/tcp-rx-buffer.h"
 #include "ns3/rtt-estimator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpEcnpTryTestSuite");

/**
 * \ingroup internet-test
 * \ingroup tests
 *
 * \brief A TCP socket which sends certain control packets with CE flags.
 *
 * The SendEmptyPacket function of this class is to construct CE SYN/ACK for case 6 and 7.
 *
 */
class TcpSocketCongestionRouter : public TcpSocketMsgBase
{
public:
  static TypeId GetTypeId (void);

  TcpSocketCongestionRouter () : TcpSocketMsgBase ()
  {
    m_controlPacketSent = 0;
  }

  TcpSocketCongestionRouter (const TcpSocketCongestionRouter &other)
    : TcpSocketMsgBase (other),
      m_testcase (other.m_testcase),
      m_who (other.m_who)
  {
  }

  enum SocketWho
  {
    SENDER,  //!< Sender node
    RECEIVER //!< Receiver node
  };

  void SetTestCase (uint32_t testCase, SocketWho who);

protected:
  virtual void SendEmptyPacket (uint8_t flags,bool markect);
  virtual Ptr<TcpSocketBase> Fork (void);
  void SetCE (Ptr<Packet> p);

public:
  uint32_t m_controlPacketSent;
  uint32_t m_testcase;
  SocketWho m_who;
};

NS_OBJECT_ENSURE_REGISTERED (TcpSocketCongestionRouter);

TypeId
TcpSocketCongestionRouter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpSocketCongestionRouter")
    .SetParent<TcpSocketMsgBase> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpSocketCongestionRouter> ()
  ;
  return tid;
}
void
TcpSocketCongestionRouter::SetTestCase (uint32_t testCase, SocketWho who)
{
  m_testcase = testCase;
  m_who = who;
}

Ptr<TcpSocketBase>
TcpSocketCongestionRouter::Fork (void)
{
  return CopyObject<TcpSocketCongestionRouter> (this);
}

/* Send an empty packet with specified TCP flags */
void
TcpSocketCongestionRouter::SendEmptyPacket (uint8_t flags,bool markect)
{
  NS_LOG_FUNCTION (this << static_cast<uint32_t> (flags));

  if (m_endPoint == nullptr && m_endPoint6 == nullptr)
    {
      NS_LOG_WARN ("Failed to send empty packet due to null endpoint");
      return;
    }

  Ptr<Packet> p = Create<Packet> ();
  TcpHeader header;
  SequenceNumber32 s = m_tcb->m_nextTxSequence;
  bool hasSyn = flags & TcpHeader::SYN;
  bool hasFin = flags & TcpHeader::FIN;
  bool isAck = flags == TcpHeader::ACK;
  bool hasAck = flags & TcpHeader::ACK;
  bool hasEce = flags & TcpHeader::ECE;


  if (flags & TcpHeader::FIN)
    {
      flags |= TcpHeader::ACK;
    }
  else if (m_state == FIN_WAIT_1 || m_state == LAST_ACK || m_state == CLOSING)
    {
      ++s;
    }
  if ((hasSyn & hasAck & hasEce & markect) && (m_testcase == 4 || m_testcase == 5 || m_testcase == 6 || m_testcase == 7))  // Marking SYN/ACK Packets of testcases of EcnpTry/TryOnce capable receivers as ECT(0)
    {
      NS_LOG_DEBUG ("Marking CE for SYN/ACK Packets");
      m_tcb->m_ecnState = TcpSocketState::ECN_IDLE;
      AddSocketTags (p,true);
      if (m_testcase == 6 || m_testcase == 7)
        {
          SetCE (p);   // Marking the SYN/ACK Packet as CE marked for testcases 6 and 7
        }
    }
  else
    {
      AddSocketTags (p,false);
    }
  header.SetFlags (flags);
  header.SetSequenceNumber (s);
  header.SetAckNumber (m_rxBuffer->NextRxSequence ());
  if (m_endPoint != nullptr)
    {
      header.SetSourcePort (m_endPoint->GetLocalPort ());
      header.SetDestinationPort (m_endPoint->GetPeerPort ());
    }
  else
    {
      header.SetSourcePort (m_endPoint6->GetLocalPort ());
      header.SetDestinationPort (m_endPoint6->GetPeerPort ());
    }
  AddOptions (header);

  // RFC 6298, clause 2.4
  m_rto = Max (m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation () * 4), m_minRto);

  uint16_t windowSize = AdvertisedWindowSize ();
  if (hasSyn)
    {
      if (m_winScalingEnabled)
        { // The window scaling option is set only on SYN packets
          AddOptionWScale (header);
        }

      if (m_sackEnabled)
        {
          AddOptionSackPermitted (header);
        }

      if (m_synCount == 0)
        { // No more connection retries, give up
          NS_LOG_LOGIC ("Connection failed.");
          m_rtt->Reset (); // According to recommendation -> RFC 6298
          CloseAndNotify ();
          return;
        }
      else
        { // Exponential backoff of connection time out
          int backoffCount = 0x1 << (m_synRetries - m_synCount);
          m_rto = m_cnTimeout * backoffCount;
          m_synCount--;
        }

      if (m_synRetries - 1 == m_synCount)
        {
          UpdateRttHistory (s, 0, false);
        }
      else
        { // This is SYN retransmission
          UpdateRttHistory (s, 0, true);
        }

      windowSize = AdvertisedWindowSize (false);
    }
  header.SetWindowSize (windowSize);

  if (flags & TcpHeader::ACK)
    { // If sending an ACK, cancel the delay ACK as well
      m_delAckEvent.Cancel ();
      m_delAckCount = 0;
      if (m_highTxAck < header.GetAckNumber ())
        {
          m_highTxAck = header.GetAckNumber ();
        }
      if (m_sackEnabled && m_rxBuffer->GetSackListSize () > 0)
        {
          AddOptionSack (header);
        }
      NS_LOG_INFO ("Sending a pure ACK, acking seq " << m_rxBuffer->NextRxSequence ());
    }

  m_txTrace (p, header, this);

  if (m_endPoint != nullptr)
    {
      m_tcp->SendPacket (p, header, m_endPoint->GetLocalAddress (),
                         m_endPoint->GetPeerAddress (), m_boundnetdevice);
    }
  else
    {
      m_tcp->SendPacket (p, header, m_endPoint6->GetLocalAddress (),
                         m_endPoint6->GetPeerAddress (), m_boundnetdevice);
    }


  if (m_retxEvent.IsExpired () && (hasSyn || hasFin) && !isAck )
    { // Retransmit SYN / SYN+ACK / FIN / FIN+ACK to guard against lost
      NS_LOG_LOGIC ("Schedule retransmission timeout at time "
                    << Simulator::Now ().GetSeconds () << " to expire at time "
                    << (Simulator::Now () + m_rto.Get ()).GetSeconds ());
      m_retxEvent = Simulator::Schedule (m_rto, &TcpSocketCongestionRouter::SendEmptyPacket, this, flags,false);
    }
}

void TcpSocketCongestionRouter::SetCE (Ptr<Packet> p)
{
  SocketIpTosTag ipTosTag;
  ipTosTag.SetTos (MarkEcnCe (0));
  p->ReplacePacketTag (ipTosTag);

  SocketIpv6TclassTag ipTclassTag;
  ipTclassTag.SetTclass (MarkEcnCe (0));
  p->ReplacePacketTag (ipTclassTag);
}

/**
 * \ingroup internet-test
 * \ingroup tests
 *
 * \brief checks if ECT (in IP header), CWR and ECE (in TCP header) bits are set correctly in different patcket types.
 *
 * This test suite will run five combinations of ClassicEcn to different Ecn Modes.
 * case 1: SENDER EcnpTry       RECEIVER NoEcn
 * case 2: SENDER EcnpTry       RECEIVER ClassicEcn
 * case 3: SENDER NoEcn       RECEIVER EcnpTry
 * case 4: SENDER ClassicEcn  RECEIVER EcnpTry
 * case 5: SENDER EcnpTry       RECEIVER EcnpTry
 * The first five cases are trying to test the following things:
 * 1. ECT, CWR and ECE setting correctness in SYN, SYN/ACK and ACK in negotiation phases
 * To test the congestion response for SYN/ACK packet, case 6 and 7 are constructed.
 * case 6: SENDER ClassicEcn  RECEIVER EcnpTry
 * case 7: SENDER EcnpTry       RECEIVER EcnpTry
 * case 6 should ignore the CE mark in SYN/ACK when sender received this packet
 * cass 7 will feedback this information from initiator to responder
 * The sender sends an ACK+ECE to the responder indicating congestion in the network. 
 * Responder responds to the congestion indication reducing IW to 1 SMSS  and sending another SYN/ACK packet with not-ect in IP header.
 * The sender sends another ACK thus establishing the connectin with the responder.
 */
class TcpEcnpTry : public TcpGeneralTest
{
public:
  /**
   * \brief Constructor
   *
   * \param testcase test case number
   * \param desc Description about the ECN capabilities of sender and reciever
   */
  TcpEcnpTry (uint32_t testcase, const std::string &desc);

protected:
  virtual void Rx (const Ptr<const Packet> p, const TcpHeader&h, SocketWho who);
  virtual void Tx (const Ptr<const Packet> p, const TcpHeader&h, SocketWho who);
  virtual void CWndTrace (uint32_t oldValue, uint32_t newValue);
  virtual Ptr<TcpSocketMsgBase> CreateSenderSocket (Ptr<Node> node);
  virtual Ptr<TcpSocketMsgBase> CreateReceiverSocket (Ptr<Node> node);
  void ConfigureProperties ();

private:
  uint32_t m_testcase;
  uint32_t m_senderSent;
  uint32_t m_senderReceived;
  uint32_t m_receiverSent;
  uint32_t m_receiverReceived;
  uint32_t m_cwndChangeCount;
};

TcpEcnpTry::TcpEcnpTry (uint32_t testcase, const std::string &desc)
  : TcpGeneralTest (desc),
    m_testcase (testcase),
    m_senderSent (0),
    m_senderReceived (0),
    m_receiverSent (0),
    m_receiverReceived (0),
    m_cwndChangeCount (0)
{

}

void TcpEcnpTry::ConfigureProperties ()
{
  TcpGeneralTest::ConfigureProperties ();
  if (m_testcase == 1 || m_testcase == 2 || m_testcase == 5 || m_testcase == 7)
    {
      SetEcn (SENDER, TcpSocketBase::EcnpTry);
    }
  else if (m_testcase == 4 || m_testcase == 6)
    {
      SetEcn (SENDER, TcpSocketBase::ClassicEcn);
    }

  if (m_testcase == 3 || m_testcase == 4 || m_testcase == 5 || m_testcase == 6 || m_testcase == 7)
    {
      SetEcn (RECEIVER, TcpSocketBase::EcnpTry);
    }
  else if (m_testcase == 2)
    {
      SetEcn (RECEIVER, TcpSocketBase::ClassicEcn);
    }
}

Ptr<TcpSocketMsgBase> TcpEcnpTry::CreateSenderSocket (Ptr<Node> node)
{
  Ptr<TcpSocketCongestionRouter> socket = DynamicCast<TcpSocketCongestionRouter> (
      CreateSocket (node, TcpSocketCongestionRouter::GetTypeId (), m_congControlTypeId));
  socket->SetTestCase (m_testcase, TcpSocketCongestionRouter::SENDER);
  return socket;
}

Ptr<TcpSocketMsgBase> TcpEcnpTry::CreateReceiverSocket (Ptr<Node> node)
{
  Ptr<TcpSocketCongestionRouter> socket = DynamicCast<TcpSocketCongestionRouter> (
      CreateSocket (node, TcpSocketCongestionRouter::GetTypeId (), m_congControlTypeId));
  socket->SetTestCase (m_testcase, TcpSocketCongestionRouter::RECEIVER);
  return socket;
}

// This function verifies whether the responder reduces the congestion window size to 1 SMSS on receiving congestion notification for SYN/ACK Packet.
void
TcpEcnpTry::CWndTrace (uint32_t oldValue, uint32_t newValue)
{
  if (m_testcase == 7)
    {
      if (newValue < oldValue)
        {
          m_cwndChangeCount++;
          NS_LOG_DEBUG ("Congestion Window Size reduced in response to ACK+ECE during negotiation");
          NS_TEST_ASSERT_MSG_EQ (m_cwndChangeCount, 1, "Congestion window should be reduced once per every window");
          NS_TEST_ASSERT_MSG_EQ (newValue, 1, "Congestion window should be reduced to 1 SMSS");
        }
    }
}

void
TcpEcnpTry::Rx (const Ptr<const Packet> p, const TcpHeader &h, SocketWho who)
{
  NS_LOG_FUNCTION (this << m_testcase << who);

  if (who == RECEIVER)
    {
      m_receiverReceived++;
      NS_LOG_DEBUG ("Packet size is: " << p->GetSize () << " at packet: " << m_receiverReceived);
      if (m_receiverReceived == 1) // SYN for negotiation test in TCP header
        {
          NS_TEST_ASSERT_MSG_NE (((h.GetFlags ()) & TcpHeader::SYN), 0, "SYN should be received as first message at the receiver");

          if (m_testcase == 1 || m_testcase == 2 || m_testcase == 4 ||m_testcase == 5 ||m_testcase == 6 ||m_testcase == 7)
            {
              NS_TEST_ASSERT_MSG_NE (((h.GetFlags ()) & TcpHeader::ECE) && ((h.GetFlags ()) & TcpHeader::CWR), 0, "The flags ECE + CWR should be set in the TCP header of SYN when sender is ECN Capable");
              NS_LOG_DEBUG ("SYN+ECE+CWR Received as first message at receiver");
            }
          else if (m_testcase == 3)
            {
              NS_TEST_ASSERT_MSG_EQ (((h.GetFlags ()) & TcpHeader::ECE) || ((h.GetFlags ()) & TcpHeader::CWR), 0, "The flags ECE + CWR should not be set in the TCP header of SYN at receiver when sender is not ECN Capable");
            }

        }

      if (m_receiverReceived == 2) // ACK for negotiation test in TCP header
        {
          NS_TEST_ASSERT_MSG_NE (((h.GetFlags ()) & TcpHeader::ACK), 0, "ACK should be received as second message at receiver");
          if (m_testcase == 1 || m_testcase == 2 || m_testcase == 3 || m_testcase == 4 || m_testcase == 5)
            {
              NS_TEST_ASSERT_MSG_EQ (((h.GetFlags ()) & TcpHeader::ECE), 0, "ECE should not be set if the SYN/ACK not CE in any cases");
            }

          // Test if ACK with ECE in TCP header when received SYN/ACK with CE mark
          if (m_testcase == 6)
            {
              NS_TEST_ASSERT_MSG_EQ (((h.GetFlags ()) & TcpHeader::ECE), 0, "ECE should not be set if the sender is classicECN when received SYN/ACK with CE mark");
            }
          else if (m_testcase == 7)
            {
              NS_LOG_DEBUG ("ACK+ECE should be received as the message after receiving the CE marked SYN/ACK packet");
              NS_TEST_ASSERT_MSG_NE (((h.GetFlags ()) & TcpHeader::ECE), 0, "ECE should be set if the sender is EcnPTry when received SYN/ACK with CE mark");
            }
        }
      // Test if ACK with ECE in TCP header when received SYN/ACK with CE mark
      if (m_testcase == 7)
        {
          if (m_receiverReceived == 3) // ACK for not-ect SYN/ACK packet
            {
              NS_TEST_ASSERT_MSG_NE (((h.GetFlags ()) & TcpHeader::ACK), 0, "ACK should be received for the second time at receiver without ECE");
              NS_TEST_ASSERT_MSG_EQ (((h.GetFlags ()) & TcpHeader::ECE), 0, "ACK should be received for the second time at receiver without ECE");
            }
        }
    }

  if (who == SENDER)
    {
      m_senderReceived++;
      if (m_senderReceived == 1) // SYN/ACK for negotiation test in TCP header
        {
          NS_TEST_ASSERT_MSG_NE (((h.GetFlags ()) & TcpHeader::SYN) && ((h.GetFlags ()) & TcpHeader::ACK), 0, "SYN+ACK received as first message at sender");
          if (m_testcase == 2 || m_testcase == 4 || m_testcase == 5|| m_testcase == 6 || m_testcase == 7)
            {
              NS_LOG_DEBUG ("SYN+ACK+ECE Received as first message at sender");
              NS_TEST_ASSERT_MSG_NE ((h.GetFlags () & TcpHeader::ECE), 0, "The flag ECE should be set in the TCP header of SYN/ACK at sender when both receiver and sender are ECN Capable");
            }
          else if (m_testcase == 1 || m_testcase == 3)
            {
              NS_TEST_ASSERT_MSG_EQ (((h.GetFlags ()) & TcpHeader::ECE), 0, "The flag ECE should not be set in the TCP header of SYN/ACK at sender when either receiver or sender are not ECN Capable");
            }
        }


      if (m_testcase == 7)
        {
          if (m_senderReceived == 2) // Second SYN/ACK for negotiation in TCP header
            {
              NS_TEST_ASSERT_MSG_NE ( ((h.GetFlags ()) & TcpHeader::SYN) && ((h.GetFlags ()) & TcpHeader::ACK) && ((h.GetFlags ()) & TcpHeader::ECE), 0, "SYN+ACK with not-ect received as second message at sender");
            }
        }
    }
}

void TcpEcnpTry::Tx (const Ptr<const Packet> p, const TcpHeader &h, SocketWho who)
{
  NS_LOG_FUNCTION (this << m_testcase << who);
  SocketIpTosTag ipTosTag;
  p->PeekPacketTag (ipTosTag);
  uint16_t ipTos = static_cast<uint16_t> (ipTosTag.GetTos () & 0x3);
  if (who == SENDER)
    {
      m_senderSent++;
      if (m_senderSent == 1) // SYN for negotiation test in IP header
        {
          if (m_testcase == 1 || m_testcase == 2 || m_testcase == 3 || m_testcase == 4 || m_testcase == 5 || m_testcase == 6 || m_testcase == 7)
            {
              NS_TEST_ASSERT_MSG_EQ (ipTos, 0x0, "IP TOS should not have ECT set in SYN");
            }
        }

      if (m_senderSent == 2) // ACK for negotiation test in IP header
        {
          if (m_testcase == 1 || m_testcase == 2 || m_testcase == 3 || m_testcase == 4 || m_testcase == 5 || m_testcase == 6 || m_testcase == 7)
            {
              NS_TEST_ASSERT_MSG_EQ (ipTos, 0x0, "IP TOS should not have ECT set in pure ACK");
            }
        }

      if (m_senderSent == 3) // ACK for negotiation test in IP header
        {
          if (m_testcase == 7)
            {
              NS_TEST_ASSERT_MSG_EQ (ipTos, 0x0, "IP TOS should not have ECT set in pure ACK");
            }
        }
    }
  if (who == RECEIVER)
    {
      m_receiverSent++;
      if (m_receiverSent == 1) // SYN/ACK for negotiation test
        {
          if (m_testcase == 4 || m_testcase == 5 )
            {
              NS_TEST_ASSERT_MSG_EQ (ipTos, 0x2, "IP TOS should have ECT set in SYN/ACK");
            }
          if (m_testcase == 7 || m_testcase == 6)
            {
              NS_TEST_ASSERT_MSG_EQ (ipTos, 0x3, "IP TOS should have CE set in SYN/ACK");     // Verifying CE code point in SYN/ACK packet
            }

          else if (m_testcase == 1 || m_testcase == 2 || m_testcase == 3)
            {
              NS_TEST_ASSERT_MSG_EQ (ipTos, 0x0, "IP TOS should not have ECT set in SYN/ACK");
            }
        }
      if (m_testcase == 7)
        {
          if (m_receiverSent == 2) // SYN/ACK for negotiation test
            {
              NS_TEST_ASSERT_MSG_EQ (ipTos, 0x0, "IP TOS should not have ECT set in SYN/ACK sent in response to ACK+ECE");
            }
        }
    }
}
/**
 * \ingroup internet-test
 * \ingroup tests
 *
 * \brief TCP EcnpTry TestSuite
 */
static class TcpEcnpTrySuite : public TestSuite
{
public:
  TcpEcnpTrySuite () : TestSuite ("tcp-ecnptry-test", UNIT)
  {
    AddTestCase (new TcpEcnpTry (1, "EcnpTry Negotiation Test : EcnpTry capable sender and ECN incapable receiver"),
                 TestCase::QUICK);
    AddTestCase (new TcpEcnpTry (2, "EcnpTry Negotiation Test : EcnpTry capable sender and classicECN capable receiver"),
                 TestCase::QUICK);
    AddTestCase (new TcpEcnpTry (3, "EcnpTry Negotiation Test : ECN incapable sender and EcnpTry capable receiver"),
                 TestCase::QUICK);
    AddTestCase (new TcpEcnpTry (4, "EcnpTry Negotiation Test : classicECN capable sender and EcnpTry capable receiver"),
                 TestCase::QUICK);
    AddTestCase (new TcpEcnpTry (5, "EcnpTry Negotiation Test : EcnpTry capable sender and EcnpTry capable receiver"),
                 TestCase::QUICK);
    AddTestCase (new TcpEcnpTry (6, "EcnpTry SYN+ACK CE Test : classicECN capable sender and EcnpTry capable receiver"),
                 TestCase::QUICK);
    AddTestCase (new TcpEcnpTry (7, "EcnpTry SYN+ACK CE Test : EcnpTry capable sender and EcnpTry capable receiver"),
                 TestCase::QUICK);
  }
} g_ecnpTestSuite;
}
// namespace ns3

