/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009, 2010 MIRKO BANCHI
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
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */

#include "ns3/test.h"
#include "ns3/string.h"
#include "ns3/qos-utils.h"
#include "ns3/ctrl-headers.h"
#include "ns3/packet.h"
#include "ns3/wifi-net-device.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/mobility-helper.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/config.h"
#include "ns3/pointer.h"

using namespace ns3;

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Packet Buffering Case A
 *
 * This simple test verifies the correctness of buffering for packets received
 * under block ack. In order to completely understand this example is important to cite
 * section 9.10.3 in IEEE802.11 standard:
 *
 * "[...] The sequence number space is considered divided into two parts, one of which
 * is “old” and one of which is “new” by means of a boundary created by adding half the
 * sequence number range to the current start of receive window (modulo 2^12)."
 */
//-------------------------------------------------------------------------------------

/* ----- = old packets
 * +++++ = new packets
 *
 *  CASE A: startSeq < endSeq
 *                        -  -   +
 *  initial buffer state: 0 16 56000
 *
 *
 *    0                            4095
 *    |------|++++++++++++++++|-----|
 *           ^                ^
 *           | startSeq       | endSeq = 4000
 *
 *  first received packet's sequence control = 64016 (seqNum = 4001, fragNum = 0) -
 *  second received packet's sequence control = 63984 (seqNum = 3999, fragNum = 0) +
 *  4001 is older seq number so this packet should be inserted at the buffer's begin.
 *  3999 is previous element of older of new packets: it should be inserted at the end of buffer.
 *
 *  expected buffer state: 64016 0 16 56000 63984
 *
 */
class PacketBufferingCaseA : public TestCase
{
public:
  PacketBufferingCaseA ();
  virtual ~PacketBufferingCaseA ();
private:
  virtual void DoRun (void);
  std::list<uint16_t> m_expectedBuffer; ///< expected test buffer
};

PacketBufferingCaseA::PacketBufferingCaseA ()
  : TestCase ("Check correct order of buffering when startSequence < endSeq")
{
  m_expectedBuffer.push_back (64016);
  m_expectedBuffer.push_back (0);
  m_expectedBuffer.push_back (16);
  m_expectedBuffer.push_back (56000);
  m_expectedBuffer.push_back (63984);
}

PacketBufferingCaseA::~PacketBufferingCaseA ()
{
}

void
PacketBufferingCaseA::DoRun (void)
{
  std::list<uint16_t> m_buffer;
  std::list<uint16_t>::iterator i,j;
  m_buffer.push_back (0);
  m_buffer.push_back (16);
  m_buffer.push_back (56000);

  uint16_t endSeq = 4000;

  uint16_t receivedSeq = 4001 * 16;
  uint32_t mappedSeq = QosUtilsMapSeqControlToUniqueInteger (receivedSeq, endSeq);
  /* cycle to right position for this packet */
  for (i = m_buffer.begin (); i != m_buffer.end (); i++)
    {
      if (QosUtilsMapSeqControlToUniqueInteger ((*i), endSeq) >= mappedSeq)
        {
          //position found
          break;
        }
    }
  m_buffer.insert (i, receivedSeq);

  receivedSeq = 3999 * 16;
  mappedSeq = QosUtilsMapSeqControlToUniqueInteger (receivedSeq, endSeq);
  /* cycle to right position for this packet */
  for (i = m_buffer.begin (); i != m_buffer.end (); i++)
    {
      if (QosUtilsMapSeqControlToUniqueInteger ((*i), endSeq) >= mappedSeq)
        {
          //position found
          break;
        }
    }
  m_buffer.insert (i, receivedSeq);

  for (i = m_buffer.begin (), j = m_expectedBuffer.begin (); i != m_buffer.end (); i++, j++)
    {
      NS_TEST_EXPECT_MSG_EQ (*i, *j, "error in buffer order");
    }
}


/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Packet Buffering Case B
 *
 * ----- = old packets
 * +++++ = new packets
 *
 *  CASE B: startSeq > endSeq
 *                         -    +    +
 *  initial buffer state: 256 64000 16
 *
 *
 *    0                            4095
 *    |++++++|----------------|++++++|
 *           ^                ^
 *           | endSeq = 10    | startSeq
 *
 *  first received packet's sequence control = 240 (seqNum = 15, fragNum = 0)  -
 *  second received packet's sequence control = 241 (seqNum = 15, fragNum = 1) -
 *  third received packet's sequence control = 64800 (seqNum = 4050, fragNum = 0) +
 *  240 is an old packet should be inserted at the buffer's begin.
 *  241 is an old packet: second segment of the above packet.
 *  4050 is a new packet: it should be inserted between 64000 and 16.
 *
 *  expected buffer state: 240 241 256 64000 64800 16
 *
 */
class PacketBufferingCaseB : public TestCase
{
public:
  PacketBufferingCaseB ();
  virtual ~PacketBufferingCaseB ();
private:
  virtual void DoRun (void);
  std::list<uint16_t> m_expectedBuffer; ///< expected test buffer
};

PacketBufferingCaseB::PacketBufferingCaseB ()
  : TestCase ("Check correct order of buffering when startSequence > endSeq")
{
  m_expectedBuffer.push_back (240);
  m_expectedBuffer.push_back (241);
  m_expectedBuffer.push_back (256);
  m_expectedBuffer.push_back (64000);
  m_expectedBuffer.push_back (64800);
  m_expectedBuffer.push_back (16);
}

PacketBufferingCaseB::~PacketBufferingCaseB ()
{
}

void
PacketBufferingCaseB::DoRun (void)
{
  std::list<uint16_t> m_buffer;
  std::list<uint16_t>::iterator i,j;
  m_buffer.push_back (256);
  m_buffer.push_back (64000);
  m_buffer.push_back (16);

  uint16_t endSeq = 10;

  uint16_t receivedSeq = 15 * 16;
  uint32_t mappedSeq = QosUtilsMapSeqControlToUniqueInteger (receivedSeq, endSeq);
  /* cycle to right position for this packet */
  for (i = m_buffer.begin (); i != m_buffer.end (); i++)
    {
      if (QosUtilsMapSeqControlToUniqueInteger ((*i), endSeq) >= mappedSeq)
        {
          //position found
          break;
        }
    }
  m_buffer.insert (i, receivedSeq);

  receivedSeq = 15 * 16 + 1;
  mappedSeq = QosUtilsMapSeqControlToUniqueInteger (receivedSeq, endSeq);
  /* cycle to right position for this packet */
  for (i = m_buffer.begin (); i != m_buffer.end (); i++)
    {
      if (QosUtilsMapSeqControlToUniqueInteger ((*i), endSeq) >= mappedSeq)
        {
          //position found
          break;
        }
    }
  m_buffer.insert (i, receivedSeq);

  receivedSeq = 4050 * 16;
  mappedSeq = QosUtilsMapSeqControlToUniqueInteger (receivedSeq, endSeq);
  /* cycle to right position for this packet */
  for (i = m_buffer.begin (); i != m_buffer.end (); i++)
    {
      if (QosUtilsMapSeqControlToUniqueInteger ((*i), endSeq) >= mappedSeq)
        {
          //position found
          break;
        }
    }
  m_buffer.insert (i, receivedSeq);

  for (i = m_buffer.begin (), j = m_expectedBuffer.begin (); i != m_buffer.end (); i++, j++)
    {
      NS_TEST_EXPECT_MSG_EQ (*i, *j, "error in buffer order");
    }
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test for the originator block ack window
 */
class OriginatorBlockAckWindowTest : public TestCase
{
public:
  OriginatorBlockAckWindowTest ();
private:
  virtual void DoRun ();
};

OriginatorBlockAckWindowTest::OriginatorBlockAckWindowTest ()
  : TestCase ("Check the correctness of the originator block ack window")
{
}

void
OriginatorBlockAckWindowTest::DoRun (void)
{
  uint16_t winSize = 16;
  uint16_t startingSeq = 4090;

  OriginatorBlockAckAgreement agreement (Mac48Address ("00:00:00:00:00:01"), 0);
  agreement.SetBufferSize (winSize);
  agreement.SetStartingSequence (startingSeq);
  agreement.InitTxWindow ();

  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.GetWinSize (), winSize, "Incorrect window size");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.GetWinStart (), startingSeq, "Incorrect winStart");
  // check that all the elements in the window are cleared
  for (uint16_t i = 0; i < winSize; i++)
    {
      NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (i), false, "Not all flags are cleared after initialization");
    }

  // Notify the acknowledgment of 5 packets
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_QOSDATA);
  Ptr<WifiMacQueueItem> mpdu = Create<WifiMacQueueItem> (Create<Packet> (), hdr);
  uint16_t seqNumber = startingSeq;
  mpdu->GetHeader ().SetSequenceNumber (seqNumber);
  agreement.NotifyAckedMpdu (mpdu);

  mpdu->GetHeader ().SetSequenceNumber (++seqNumber %= SEQNO_SPACE_SIZE);
  agreement.NotifyAckedMpdu (mpdu);

  mpdu->GetHeader ().SetSequenceNumber (++seqNumber %= SEQNO_SPACE_SIZE);
  agreement.NotifyAckedMpdu (mpdu);

  mpdu->GetHeader ().SetSequenceNumber (++seqNumber %= SEQNO_SPACE_SIZE);
  agreement.NotifyAckedMpdu (mpdu);

  mpdu->GetHeader ().SetSequenceNumber (++seqNumber %= SEQNO_SPACE_SIZE);
  agreement.NotifyAckedMpdu (mpdu);

  // the current window must look like this:
  //
  // |0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
  //            ^
  //            |
  //           HEAD

  startingSeq = (seqNumber + 1) % SEQNO_SPACE_SIZE;
  NS_TEST_EXPECT_MSG_EQ (agreement.GetStartingSequence (), startingSeq, "Incorrect starting sequence after 5 acknowledgments");
  for (uint16_t i = 0; i < winSize; i++)
    {
      NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (i), false, "Not all flags are cleared after 5 acknowledgments");
    }

  // the next MPDU is not acknowledged, hence the window is blocked while the
  // subsequent 4 MPDUs are acknowledged
  ++seqNumber %= SEQNO_SPACE_SIZE;
  mpdu->GetHeader ().SetSequenceNumber (++seqNumber %= SEQNO_SPACE_SIZE);
  agreement.NotifyAckedMpdu (mpdu);

  mpdu->GetHeader ().SetSequenceNumber (++seqNumber %= SEQNO_SPACE_SIZE);
  agreement.NotifyAckedMpdu (mpdu);

  mpdu->GetHeader ().SetSequenceNumber (++seqNumber %= SEQNO_SPACE_SIZE);
  agreement.NotifyAckedMpdu (mpdu);

  mpdu->GetHeader ().SetSequenceNumber (++seqNumber %= SEQNO_SPACE_SIZE);
  agreement.NotifyAckedMpdu (mpdu);

  // the current window must look like this:
  //
  // |0|0|0|0|0|0|1|1|1|1|0|0|0|0|0|0|
  //            ^
  //            |
  //           HEAD

  NS_TEST_EXPECT_MSG_EQ (agreement.GetStartingSequence (), startingSeq, "Incorrect starting sequence after 1 unacknowledged MPDU");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (0), false, "Incorrect flag after 1 unacknowledged MPDU");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (1), true, "Incorrect flag after 1 unacknowledged MPDU");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (2), true, "Incorrect flag after 1 unacknowledged MPDU");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (3), true, "Incorrect flag after 1 unacknowledged MPDU");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (4), true, "Incorrect flag after 1 unacknowledged MPDU");
  for (uint16_t i = 5; i < winSize; i++)
    {
      NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (i), false, "Incorrect flag after 1 unacknowledged MPDU");
    }

  // the missing MPDU is now acknowledged; the window moves forward and the starting
  // sequence number is the one of the first unacknowledged MPDU
  mpdu->GetHeader ().SetSequenceNumber (startingSeq);
  agreement.NotifyAckedMpdu (mpdu);

  // the current window must look like this:
  //
  // |0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
  //                      ^
  //                      |
  //                     HEAD

  startingSeq = (seqNumber + 1) % SEQNO_SPACE_SIZE;
  NS_TEST_EXPECT_MSG_EQ (agreement.GetStartingSequence (), startingSeq, "Incorrect starting sequence after acknowledgment of missing MPDU");
  for (uint16_t i = 0; i < winSize; i++)
    {
      NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (i), false, "Not all flags are cleared after acknowledgment of missing MPDU");
    }

  // Now, create a hole of 3 MPDUs before 4 acknowledged MPDUs, another hole of 2 MPDUs before 3 acknowledged MPDUs
  seqNumber = (seqNumber + 4) % SEQNO_SPACE_SIZE;
  mpdu->GetHeader ().SetSequenceNumber (seqNumber);
  agreement.NotifyAckedMpdu (mpdu);

  mpdu->GetHeader ().SetSequenceNumber (++seqNumber %= SEQNO_SPACE_SIZE);
  agreement.NotifyAckedMpdu (mpdu);

  mpdu->GetHeader ().SetSequenceNumber (++seqNumber %= SEQNO_SPACE_SIZE);
  agreement.NotifyAckedMpdu (mpdu);

  mpdu->GetHeader ().SetSequenceNumber (++seqNumber %= SEQNO_SPACE_SIZE);
  agreement.NotifyAckedMpdu (mpdu);

  seqNumber = (seqNumber + 3) % SEQNO_SPACE_SIZE;
  mpdu->GetHeader ().SetSequenceNumber (seqNumber);
  agreement.NotifyAckedMpdu (mpdu);

  mpdu->GetHeader ().SetSequenceNumber (++seqNumber %= SEQNO_SPACE_SIZE);
  agreement.NotifyAckedMpdu (mpdu);

  mpdu->GetHeader ().SetSequenceNumber (++seqNumber %= SEQNO_SPACE_SIZE);
  agreement.NotifyAckedMpdu (mpdu);

  // the current window must look like this:
  //
  // |1|0|0|1|1|1|0|0|0|0|0|0|0|1|1|1|
  //                      ^
  //                      |
  //                     HEAD

  NS_TEST_EXPECT_MSG_EQ (agreement.GetStartingSequence (), startingSeq, "Incorrect starting sequence after 3 unacknowledged MPDUs");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (0), false, "Incorrect flag after 3 unacknowledged MPDUs");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (1), false, "Incorrect flag after 3 unacknowledged MPDUs");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (2), false, "Incorrect flag after 3 unacknowledged MPDUs");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (3), true, "Incorrect flag after 3 unacknowledged MPDUs");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (4), true, "Incorrect flag after 3 unacknowledged MPDUs");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (5), true, "Incorrect flag after 3 unacknowledged MPDUs");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (6), true, "Incorrect flag after 3 unacknowledged MPDUs");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (7), false, "Incorrect flag after 3 unacknowledged MPDUs");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (8), false, "Incorrect flag after 3 unacknowledged MPDUs");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (9), true, "Incorrect flag after 3 unacknowledged MPDUs");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (10), true, "Incorrect flag after 3 unacknowledged MPDUs");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (11), true, "Incorrect flag after 3 unacknowledged MPDUs");
  for (uint16_t i = 12; i < winSize; i++)
    {
      NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (i), false, "Incorrect flag after 3 unacknowledged MPDUs");
    }

  // the transmission of an MPDU beyond the current window (by 2 positions) is
  // notified, hence the window moves forward 2 positions
  seqNumber = (agreement.m_txWindow.GetWinEnd () + 2) % SEQNO_SPACE_SIZE;
  mpdu->GetHeader ().SetSequenceNumber (seqNumber);
  agreement.NotifyTransmittedMpdu (mpdu);

  // the current window must look like this:
  //
  // |1|0|0|1|1|1|0|0|0|0|0|0|0|1|1|1|
  //                          ^
  //                          |
  //                         HEAD

  startingSeq = (startingSeq + 2) % SEQNO_SPACE_SIZE;
  NS_TEST_EXPECT_MSG_EQ (agreement.GetStartingSequence (), startingSeq,
                         "Incorrect starting sequence after transmitting an MPDU beyond the current window");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (0), false, "Incorrect flag after transmitting an MPDU beyond the current window");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (1), true, "Incorrect flag after transmitting an MPDU beyond the current window");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (2), true, "Incorrect flag after transmitting an MPDU beyond the current window");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (3), true, "Incorrect flag after transmitting an MPDU beyond the current window");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (4), true, "Incorrect flag after transmitting an MPDU beyond the current window");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (5), false, "Incorrect flag after transmitting an MPDU beyond the current window");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (6), false, "Incorrect flag after transmitting an MPDU beyond the current window");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (7), true, "Incorrect flag after transmitting an MPDU beyond the current window");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (8), true, "Incorrect flag after transmitting an MPDU beyond the current window");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (9), true, "Incorrect flag after transmitting an MPDU beyond the current window");
  for (uint16_t i = 10; i < winSize; i++)
    {
      NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (i), false, "Incorrect flag after transmitting an MPDU beyond the current window");
    }

  // another MPDU is transmitted beyond the current window. Now, the window advances
  // until the first unacknowledged MPDU
  seqNumber = (agreement.m_txWindow.GetWinEnd () + 1) % SEQNO_SPACE_SIZE;
  mpdu->GetHeader ().SetSequenceNumber (seqNumber);
  agreement.NotifyTransmittedMpdu (mpdu);

  // the current window must look like this:
  //
  // |0|0|0|1|1|1|0|0|0|0|0|0|0|0|0|0|
  //    ^
  //    |
  //   HEAD

  startingSeq = (startingSeq + 5) % SEQNO_SPACE_SIZE;
  NS_TEST_EXPECT_MSG_EQ (agreement.GetStartingSequence (), startingSeq,
                         "Incorrect starting sequence after transmitting another MPDU beyond the current window");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (0), false, "Incorrect flag after transmitting another MPDU beyond the current window");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (1), false, "Incorrect flag after transmitting another MPDU beyond the current window");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (2), true, "Incorrect flag after transmitting another MPDU beyond the current window");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (3), true, "Incorrect flag after transmitting another MPDU beyond the current window");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (4), true, "Incorrect flag after transmitting another MPDU beyond the current window");
  for (uint16_t i = 5; i < winSize; i++)
    {
      NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (i), false, "Incorrect flag after transmitting another MPDU beyond the current window");
    }

  // the MPDU next to winStart is discarded, hence the window advances to make it an old packet.
  // Since the subsequent MPDUs have been acknowledged, the window advances further.
  seqNumber = (startingSeq + 1) % SEQNO_SPACE_SIZE;
  mpdu->GetHeader ().SetSequenceNumber (seqNumber);
  agreement.NotifyDiscardedMpdu (mpdu);

  // the current window must look like this:
  //
  // |0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
  //              ^
  //              |
  //             HEAD

  startingSeq = (startingSeq + 5) % SEQNO_SPACE_SIZE;
  NS_TEST_EXPECT_MSG_EQ (agreement.GetStartingSequence (), startingSeq,
                         "Incorrect starting sequence after discarding an MPDU");
  for (uint16_t i = 0; i < winSize; i++)
    {
      NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (i), false, "Incorrect flag after discarding an MPDU");
    }

  // Finally, check that the window correctly advances when the MPDU with the starting sequence number
  // is acknowledged after being the only unacknowledged MPDU
  for (uint16_t i = 1; i < winSize; i++)
    {
      mpdu->GetHeader ().SetSequenceNumber ((startingSeq + i) % SEQNO_SPACE_SIZE);
      agreement.NotifyAckedMpdu (mpdu);
    }

  // the current window must look like this:
  //
  // |1|1|1|1|1|1|0|1|1|1|1|1|1|1|1|1|
  //              ^
  //              |
  //             HEAD

  NS_TEST_EXPECT_MSG_EQ (agreement.GetStartingSequence (), startingSeq,
                         "Incorrect starting sequence after acknowledging all but the first MPDU");
  NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (0), false, "Incorrect flag after acknowledging all but the first MPDU");
  for (uint16_t i = 1; i < winSize; i++)
    {
      NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (i), true, "Incorrect flag after acknowledging all but the first MPDU");
    }

  // acknowledge the first MPDU
  mpdu->GetHeader ().SetSequenceNumber (startingSeq % SEQNO_SPACE_SIZE);
  agreement.NotifyAckedMpdu (mpdu);

  // the current window must look like this:
  //
  // |0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
  //              ^
  //              |
  //             HEAD

  startingSeq = (startingSeq + winSize) % SEQNO_SPACE_SIZE;
  NS_TEST_EXPECT_MSG_EQ (agreement.GetStartingSequence (), startingSeq,
                         "Incorrect starting sequence after acknowledging the first MPDU");
  for (uint16_t i = 0; i < winSize; i++)
    {
      NS_TEST_EXPECT_MSG_EQ (agreement.m_txWindow.At (i), false, "Incorrect flag after acknowledging the first MPDU");
    }
}


/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test for block ack header
 */
class CtrlBAckResponseHeaderTest : public TestCase
{
public:
  CtrlBAckResponseHeaderTest ();
private:
  virtual void DoRun ();
  CtrlBAckResponseHeader m_blockAckHdr; ///< block ack header
};

CtrlBAckResponseHeaderTest::CtrlBAckResponseHeaderTest ()
  : TestCase ("Check the correctness of block ack compressed bitmap")
{
}

void
CtrlBAckResponseHeaderTest::DoRun (void)
{
  m_blockAckHdr.SetType (COMPRESSED_BLOCK_ACK);

  //Case 1: startSeq < endSeq
  //          179        242
  m_blockAckHdr.SetStartingSequence (179);
  for (uint16_t i = 179; i < 220; i++)
    {
      m_blockAckHdr.SetReceivedPacket (i);
    }
  for (uint16_t i = 225; i <= 242; i++)
    {
      m_blockAckHdr.SetReceivedPacket (i);
    }
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.GetCompressedBitmap (), 0xffffc1ffffffffffLL, "error in compressed bitmap");
  m_blockAckHdr.SetReceivedPacket (1500);
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.GetCompressedBitmap (), 0xffffc1ffffffffffLL, "error in compressed bitmap");
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.IsPacketReceived (220), false, "error in compressed bitmap");
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.IsPacketReceived (225), true, "error in compressed bitmap");
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.IsPacketReceived (1500), false, "error in compressed bitmap");

  m_blockAckHdr.ResetBitmap ();

  //Case 2: startSeq > endSeq
  //          4090       58
  m_blockAckHdr.SetStartingSequence (4090);
  for (uint16_t i = 4090; i != 10; i = (i + 1) % 4096)
    {
      m_blockAckHdr.SetReceivedPacket (i);
    }
  for (uint16_t i = 22; i < 25; i++)
    {
      m_blockAckHdr.SetReceivedPacket (i);
    }
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.GetCompressedBitmap (), 0x00000000007000ffffLL, "error in compressed bitmap");
  m_blockAckHdr.SetReceivedPacket (80);
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.GetCompressedBitmap (), 0x00000000007000ffffLL, "error in compressed bitmap");
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.IsPacketReceived (4090), true, "error in compressed bitmap");
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.IsPacketReceived (4095), true, "error in compressed bitmap");
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.IsPacketReceived (10), false, "error in compressed bitmap");
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.IsPacketReceived (35), false, "error in compressed bitmap");
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.IsPacketReceived (80), false, "error in compressed bitmap");
}


/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test for Block Ack Policy with aggregation disabled
 *
 * This test aims to check the Block Ack policy when A-MPDU aggregation is disabled.
 * In this case, a QoS station can transmit multiple QoS data frames before requesting
 * a Block Ack through a Block Ack Request frame. If the AC is granted a non-null TXOP
 * limit, MPDUs can be separated by a SIFS.
 *
 * In this test, an HT STA sends 14 packets to an HT AP. The ack policy selector is
 * configured so that a Block Ack is requested once 8 (= 0.125 * 64) MPDUs are sent
 * in addition to the MPDU having the starting sequence number. The block ack threshold
 * is set to 2, hence a block ack agreement is established when there are at least two
 * packets in the EDCA queue.
 *
 * When the TXOP limit is null:
 * - the first packet is sent with Normal Ack policy because the BA agreement has not
 *   been established yet (there are no queued packets when the first one arrives);
 * - packets from the 2nd to the 10th are sent with Block Ack policy (and hence
 *   are not immediately acknowledged);
 * - after the 10th packet, a Block Ack Request is sent, followed by a Block Ack;
 * - the remaining 4 packets are sent with Block Ack policy (and hence
 *   are not immediately acknowledged);
 * - the last packet is followed by a Block Ack Request because there are no more
 *   packets in the EDCA queue and hence a response is needed independently of
 *   the number of outstanding MPDUs.
 *
 * When the TXOP is not null (and long enough to include the transmission of all packets):
 * - the first packet is sent with Normal Ack policy because the BA agreement has not
 *   been established yet (there are no queued packets when the first one arrives);
 * - the second packet is sent with Normal Ack Policy because the first packet sent in
 *   a TXOP shall request an immediate response and no previous MPDUs have to be
 *   acknowledged;
 * - packets from the 3rd to the 11th are sent with Block Ack policy (and hence
 *   are not immediately acknowledged);
 * - after the 11th packet, a Block Ack Request is sent, followed by a Block Ack;
 * - the remaining 3 packets are sent with Block Ack policy (and hence
 *   are not immediately acknowledged);
 * - the last packet is followed by a Block Ack Request because there are no more
 *   packets in the EDCA queue and hence a response is needed independently of
 *   the number of outstanding MPDUs.
 */
class BlockAckAggregationDisabledTest : public TestCase
{
  /**
  * Keeps the maximum duration among all TXOPs
  */
  struct TxopDurationTracer
  {
    /**
     * Callback for the TxopTrace trace
     * \param startTime TXOP start time
     * \param duration TXOP duration
     */
    void Trace (Time startTime, Time duration);
    Time m_max {Seconds (0)};  ///< max TXOP duration
  };

public:
  /**
   * \brief Constructor
   * \param txop true for non-null TXOP limit
   */
  BlockAckAggregationDisabledTest (bool txop);
  virtual ~BlockAckAggregationDisabledTest ();

  virtual void DoRun (void);


private:
  bool m_txop; ///< true for non-null TXOP limit
  uint32_t m_received; ///< received packets
  uint16_t m_txTotal; ///< transmitted data packets
  uint16_t m_txSinceBar; ///< packets transmitted since the agreement was established
                         ///< or the last block ack was received
  uint16_t m_nBar; ///< transmitted BlockAckReq frames
  uint16_t m_nBa; ///< received BlockAck frames

  /**
   * Function to trace packets received by the server application
   * \param context the context
   * \param p the packet
   * \param adr the address
   */
  void L7Receive (std::string context, Ptr<const Packet> p, const Address &adr);
  /**
   * Callback invoked when PHY transmits a packet
   * \param context the context
   * \param p the packet
   * \param power the tx power
   */
  void Transmit (std::string context, Ptr<const Packet> p, double power);
  /**
   * Callback invoked when PHY receives a packet
   * \param context the context
   * \param p the packet
   */
  void Receive (std::string context, Ptr<const Packet> p);
};

void
BlockAckAggregationDisabledTest::TxopDurationTracer::Trace (Time startTime, Time duration)
{
  if (duration > m_max)
    {
      m_max = duration;
    }
}

BlockAckAggregationDisabledTest::BlockAckAggregationDisabledTest (bool txop)
  : TestCase ("Test case for Block Ack Policy with aggregation disabled"),
    m_txop (txop),
    m_received (0),
    m_txTotal (0),
    m_txSinceBar (0),
    m_nBar (0),
    m_nBa (0)
{
}

BlockAckAggregationDisabledTest::~BlockAckAggregationDisabledTest ()
{
}

void
BlockAckAggregationDisabledTest::L7Receive (std::string context, Ptr<const Packet> p, const Address &adr)
{
  if (p->GetSize () == 1400)
    {
      m_received++;
    }
}

void
BlockAckAggregationDisabledTest::Transmit (std::string context, Ptr<const Packet> p, double power)
{
  WifiMacHeader hdr;
  p->PeekHeader (hdr);

  if (m_txSinceBar == 9 || m_txTotal == 14)
    {
      NS_TEST_ASSERT_MSG_EQ (hdr.IsBlockAckReq (), true, "Didn't get a BlockAckReq when expected");
    }
  else
    {
      NS_TEST_ASSERT_MSG_EQ (hdr.IsBlockAckReq (), false, "Got a BlockAckReq when not expected");
    }

  if (hdr.IsQosData ())
    {
      m_txTotal++;
      if (hdr.IsQosBlockAck ())
        {
          m_txSinceBar++;
        }

      if (!m_txop)
        {
          NS_TEST_EXPECT_MSG_EQ ((m_txTotal == 1 || hdr.IsQosBlockAck ()), true, "Unexpected QoS ack policy");
        }
      else
        {
          NS_TEST_EXPECT_MSG_EQ ((m_txTotal <= 2 || hdr.IsQosBlockAck ()), true, "Unexpected QoS ack policy");
        }
    }
  else if (hdr.IsBlockAckReq ())
    {
      m_txSinceBar = 0;
      m_nBar++;
    }
}

void
BlockAckAggregationDisabledTest::Receive (std::string context, Ptr<const Packet> p)
{
  WifiMacHeader hdr;
  p->PeekHeader (hdr);

  if (hdr.IsBlockAck ())
    {
      m_nBa++;
    }
}

void
BlockAckAggregationDisabledTest::DoRun (void)
{
  NodeContainer wifiStaNode;
  wifiStaNode.Create (1);

  NodeContainer wifiApNode;
  wifiApNode.Create (1);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211a);
  wifi.SetAckPolicySelectorForAc (AC_BE, "ns3::ConstantWifiAckPolicySelector",
                                  "BaThreshold", DoubleValue (0.125));
  wifi.SetRemoteStationManager ("ns3::IdealWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "QosSupported", BooleanValue (true),
               "Ssid", SsidValue (ssid),
               /* setting blockack threshold for sta's BE queue */
               "BE_BlockAckThreshold", UintegerValue (2),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNode);

  mac.SetType ("ns3::ApWifiMac",
               "QosSupported", BooleanValue (true),
               "Ssid", SsidValue (ssid),
               "BeaconGeneration", BooleanValue (true));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (1.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);
  mobility.Install (wifiStaNode);

  Ptr<WifiNetDevice> ap_device = DynamicCast<WifiNetDevice> (apDevices.Get (0));
  Ptr<WifiNetDevice> sta_device = DynamicCast<WifiNetDevice> (staDevices.Get (0));

  // Disable A-MPDU aggregation
  sta_device->GetMac ()->SetAttribute ("BE_MaxAmpduSize", UintegerValue (0));
  TxopDurationTracer txopTracer;

  if (m_txop)
    {
      PointerValue ptr;
      sta_device->GetMac ()->GetAttribute ("BE_Txop", ptr);
      ptr.Get<QosTxop> ()->TraceConnectWithoutContext ("TxopTrace", MakeCallback (&TxopDurationTracer::Trace, &txopTracer));

      // set the TXOP limit on BE AC
      Ptr<RegularWifiMac> ap_mac = DynamicCast<RegularWifiMac> (ap_device->GetMac ());
      NS_ASSERT (ap_mac);
      ap_mac->GetAttribute ("BE_Txop", ptr);
      ptr.Get<QosTxop> ()->SetTxopLimit (MicroSeconds (4800));
    }

  PacketSocketAddress socket;
  socket.SetSingleDevice (sta_device->GetIfIndex ());
  socket.SetPhysicalAddress (ap_device->GetAddress ());
  socket.SetProtocol (1);

  // give packet socket powers to nodes.
  PacketSocketHelper packetSocket;
  packetSocket.Install (wifiStaNode);
  packetSocket.Install (wifiApNode);

  // the first client application generates a single packet, which is sent
  // with the normal ack policy because there are no other packets queued
  Ptr<PacketSocketClient> client1 = CreateObject<PacketSocketClient> ();
  client1->SetAttribute ("PacketSize", UintegerValue (1400));
  client1->SetAttribute ("MaxPackets", UintegerValue (1));
  client1->SetAttribute ("Interval", TimeValue (MicroSeconds (0)));
  client1->SetRemote (socket);
  wifiStaNode.Get (0)->AddApplication (client1);
  client1->SetStartTime (Seconds (1));
  client1->SetStopTime (Seconds (3.0));

  // the second client application generates 13 packets. Even if when the first
  // packet is queued the queue is empty, the first packet is not transmitted
  // immediately, but the EDCAF waits for the next slot boundary. At that time,
  // other packets have been queued, hence a BA agreement is established first.
  Ptr<PacketSocketClient> client2 = CreateObject<PacketSocketClient> ();
  client2->SetAttribute ("PacketSize", UintegerValue (1400));
  client2->SetAttribute ("MaxPackets", UintegerValue (13));
  client2->SetAttribute ("Interval", TimeValue (MicroSeconds (0)));
  client2->SetRemote (socket);
  wifiStaNode.Get (0)->AddApplication (client2);
  client2->SetStartTime (Seconds (1.5));
  client2->SetStopTime (Seconds (3.0));

  Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer> ();
  server->SetLocal (socket);
  wifiApNode.Get (0)->AddApplication (server);
  server->SetStartTime (Seconds (0.0));
  server->SetStopTime (Seconds (4.0));

  Config::Connect ("/NodeList/*/ApplicationList/0/$ns3::PacketSocketServer/Rx", MakeCallback (&BlockAckAggregationDisabledTest::L7Receive, this));
  Config::Connect ("/NodeList/0/DeviceList/0/Phy/PhyTxBegin", MakeCallback (&BlockAckAggregationDisabledTest::Transmit, this));
  Config::Connect ("/NodeList/0/DeviceList/0/Phy/PhyRxBegin", MakeCallback (&BlockAckAggregationDisabledTest::Receive, this));

  Simulator::Stop (Seconds (5));
  Simulator::Run ();

  Simulator::Destroy ();

  // The client applications generate 14 packets, so we expect that the wifi PHY
  // layer transmits 14 MPDUs, the server application receives 14 packets, and
  // two BARs are transmitted.
  NS_TEST_EXPECT_MSG_EQ (m_txTotal, 14, "Unexpected number of transmitted packets");
  NS_TEST_EXPECT_MSG_EQ (m_received, 14, "Unexpected number of received packets");
  NS_TEST_EXPECT_MSG_EQ (m_nBar, 2, "Unexpected number of Block Ack Requests");
  NS_TEST_EXPECT_MSG_EQ (m_nBa, 2, "Unexpected number of Block Ack Responses");
  if (m_txop)
    {
      NS_TEST_EXPECT_MSG_LT (txopTracer.m_max, MicroSeconds (4800), "TXOP duration exceeded!");
      NS_TEST_EXPECT_MSG_GT (txopTracer.m_max, MicroSeconds (3008), "The maximum TXOP duration is too short!");
    }
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Block Ack Test Suite
 */
class BlockAckTestSuite : public TestSuite
{
public:
  BlockAckTestSuite ();
};

BlockAckTestSuite::BlockAckTestSuite ()
  : TestSuite ("wifi-block-ack", UNIT)
{
  AddTestCase (new PacketBufferingCaseA, TestCase::QUICK);
  AddTestCase (new PacketBufferingCaseB, TestCase::QUICK);
  AddTestCase (new OriginatorBlockAckWindowTest, TestCase::QUICK);
  AddTestCase (new CtrlBAckResponseHeaderTest, TestCase::QUICK);
  AddTestCase (new BlockAckAggregationDisabledTest (false), TestCase::QUICK);
  AddTestCase (new BlockAckAggregationDisabledTest (true), TestCase::QUICK);
}

static BlockAckTestSuite g_blockAckTestSuite; ///< the test suite
