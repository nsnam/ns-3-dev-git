/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 University of Washington
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
 */

#include "ns3/test.h"
#include "ns3/pfifo-fast-queue-disc.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/ipv4-header.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/object-factory.h"

using namespace ns3;

/**
 * This class tests that each possible TOS is enqueued in the right band
 */
class PfifoFastQueueDiscTosPrioritization : public TestCase
{
public:
  PfifoFastQueueDiscTosPrioritization ();
  virtual ~PfifoFastQueueDiscTosPrioritization ();

private:
  virtual void DoRun (void);
  Ptr<Packet> CreatePacketWithTos (uint8_t tos);
  void TestTosValue (Ptr<PfifoFastQueueDisc> queue, uint8_t tos, uint32_t band);
};

PfifoFastQueueDiscTosPrioritization::PfifoFastQueueDiscTosPrioritization ()
  : TestCase ("Test TOS-based prioritization")
{
}

PfifoFastQueueDiscTosPrioritization::~PfifoFastQueueDiscTosPrioritization ()
{
}

Ptr<Packet>
PfifoFastQueueDiscTosPrioritization::CreatePacketWithTos (uint8_t tos)
{
  Ptr<Packet> p = Create<Packet> (100);
  Ipv4Header ipHeader;
  ipHeader.SetPayloadSize (100);
  ipHeader.SetTos (tos);
  ipHeader.SetProtocol (6);
  p->AddHeader (ipHeader);
  return p;
}

void
PfifoFastQueueDiscTosPrioritization::TestTosValue (Ptr<PfifoFastQueueDisc> queue, uint8_t tos, uint32_t band)
{
  Ptr<Packet> p = CreatePacketWithTos (tos);
  Ipv4Header ipHeader;
  p->RemoveHeader (ipHeader);
  Address dest;
  Ptr<QueueDiscItem> item = Create<QueueDiscItem> (p, ipHeader, dest, 0, 0);
  queue->Enqueue (item);
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (band), 1, "enqueued to unexpected band");
  item = queue->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (band), 0, "unable to dequeue");
}

void
PfifoFastQueueDiscTosPrioritization::DoRun (void)
{
  Ptr<PfifoFastQueueDisc> queue = CreateObject<PfifoFastQueueDisc> ();
  bool ok = queue->SetAttributeFailSafe ("Mode", EnumValue (PfifoFastQueueDisc::QUEUE_MODE_TOS));
  NS_TEST_ASSERT_MSG_EQ (ok, true, "unable to set attribute");
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (0), 0, "initialized non-zero");
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (1), 0, "initialized non-zero");
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (2), 0, "initialized non-zero");

  TestTosValue (queue, 0x0, 1);
  TestTosValue (queue, 0x2, 2);
  TestTosValue (queue, 0x4, 1);
  TestTosValue (queue, 0x6, 1);
  TestTosValue (queue, 0x8, 2);
  TestTosValue (queue, 0xa, 2);
  TestTosValue (queue, 0xc, 2);
  TestTosValue (queue, 0xe, 2);
  TestTosValue (queue, 0x10, 0);
  TestTosValue (queue, 0x12, 0);
  TestTosValue (queue, 0x14, 0);
  TestTosValue (queue, 0x16, 0);
  TestTosValue (queue, 0x18, 1);
  TestTosValue (queue, 0x1a, 1);
  TestTosValue (queue, 0x1c, 1);
  TestTosValue (queue, 0x1e, 1);
}

/**
 * This class tests that each possible DSCP is enqueued in the right band
 */
class PfifoFastQueueDiscDscpPrioritization : public TestCase
{
public:
  PfifoFastQueueDiscDscpPrioritization ();
  virtual ~PfifoFastQueueDiscDscpPrioritization ();

private:
  virtual void DoRun (void);
  Ptr<Packet> CreatePacketWithDscp (Ipv4Header::DscpType dscp);
  void TestDscpValue (Ptr<PfifoFastQueueDisc> queue, Ipv4Header::DscpType dscp, uint32_t band);
};

PfifoFastQueueDiscDscpPrioritization::PfifoFastQueueDiscDscpPrioritization ()
  : TestCase ("Test DSCP-based prioritization")
{
}

PfifoFastQueueDiscDscpPrioritization::~PfifoFastQueueDiscDscpPrioritization ()
{
}

Ptr<Packet>
PfifoFastQueueDiscDscpPrioritization::CreatePacketWithDscp (Ipv4Header::DscpType dscp)
{
  Ptr<Packet> p = Create<Packet> (100);
  Ipv4Header ipHeader;
  ipHeader.SetPayloadSize (100);
  ipHeader.SetProtocol (6);
  ipHeader.SetDscp (dscp);
  p->AddHeader (ipHeader);
  return p;
}

void
PfifoFastQueueDiscDscpPrioritization::TestDscpValue (Ptr<PfifoFastQueueDisc> queue, Ipv4Header::DscpType dscp, uint32_t band)
{
  Ptr<Packet> p = CreatePacketWithDscp (dscp);
  Ipv4Header ipHeader;
  p->RemoveHeader (ipHeader);
  Address dest;
  Ptr<QueueDiscItem> item = Create<QueueDiscItem> (p, ipHeader, dest, 0, 0);
  queue->Enqueue (item);
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (band), 1, "enqueued to unexpected band");
  item = queue->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (band), 0, "unable to dequeue");
}

void
PfifoFastQueueDiscDscpPrioritization::DoRun (void)
{
  Ptr<PfifoFastQueueDisc> queue = CreateObject<PfifoFastQueueDisc> ();
  bool ok = queue->SetAttributeFailSafe ("Mode", EnumValue (PfifoFastQueueDisc::QUEUE_MODE_DSCP));
  NS_TEST_ASSERT_MSG_EQ (ok, true, "unable to set attribute");
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (0), 0, "initialized non-zero");
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (1), 0, "initialized non-zero");
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (2), 0, "initialized non-zero");

  TestDscpValue (queue, Ipv4Header::DscpDefault, 1);
  TestDscpValue (queue, Ipv4Header::DSCP_EF, 1);
  TestDscpValue (queue, Ipv4Header::DSCP_AF11, 2);
  TestDscpValue (queue, Ipv4Header::DSCP_AF21, 2);
  TestDscpValue (queue, Ipv4Header::DSCP_AF31, 2);
  TestDscpValue (queue, Ipv4Header::DSCP_AF41, 2);
  TestDscpValue (queue, Ipv4Header::DSCP_AF12, 0);
  TestDscpValue (queue, Ipv4Header::DSCP_AF22, 0);
  TestDscpValue (queue, Ipv4Header::DSCP_AF32, 0);
  TestDscpValue (queue, Ipv4Header::DSCP_AF42, 0);
  TestDscpValue (queue, Ipv4Header::DSCP_AF13, 1);
  TestDscpValue (queue, Ipv4Header::DSCP_AF23, 1);
  TestDscpValue (queue, Ipv4Header::DSCP_AF33, 1);
  TestDscpValue (queue, Ipv4Header::DSCP_AF43, 1);
  TestDscpValue (queue, Ipv4Header::DSCP_CS1, 2);
  TestDscpValue (queue, Ipv4Header::DSCP_CS2, 1);
  TestDscpValue (queue, Ipv4Header::DSCP_CS3, 1);
  TestDscpValue (queue, Ipv4Header::DSCP_CS4, 0);
  TestDscpValue (queue, Ipv4Header::DSCP_CS5, 0);
  TestDscpValue (queue, Ipv4Header::DSCP_CS6, 0);
  TestDscpValue (queue, Ipv4Header::DSCP_CS7, 0);
}

/**
 * This class tests that each band is txqueuelen deep
 */
class PfifoFastQueueDiscOverflow : public TestCase
{
public:
  PfifoFastQueueDiscOverflow ();
  virtual ~PfifoFastQueueDiscOverflow ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<PfifoFastQueueDisc> queue, Ipv4Header::DscpType dscp);
};

PfifoFastQueueDiscOverflow::PfifoFastQueueDiscOverflow ()
  : TestCase ("Test queue overflow")
{
}

PfifoFastQueueDiscOverflow::~PfifoFastQueueDiscOverflow ()
{
}

void
PfifoFastQueueDiscOverflow::AddPacket (Ptr<PfifoFastQueueDisc> queue, Ipv4Header::DscpType dscp)
{
  Ptr<Packet> p = Create<Packet> (100);
  Ipv4Header ipHeader;
  ipHeader.SetPayloadSize (100);
  ipHeader.SetProtocol (6);
  ipHeader.SetDscp (dscp);
  Address dest;
  Ptr<QueueDiscItem> item = Create<QueueDiscItem> (p, ipHeader, dest, 0, 0);
  queue->Enqueue (item);
}

void
PfifoFastQueueDiscOverflow::DoRun (void)
{
  Ptr<DropTailQueue> band0 = CreateObjectWithAttributes<DropTailQueue> ("MaxPackets", UintegerValue (2));
  Ptr<DropTailQueue> band1 = CreateObjectWithAttributes<DropTailQueue> ("MaxPackets", UintegerValue (2));
  Ptr<DropTailQueue> band2 = CreateObjectWithAttributes<DropTailQueue> ("MaxPackets", UintegerValue (2));
  Ptr<PfifoFastQueueDisc> queue = CreateObjectWithAttributes<PfifoFastQueueDisc> ("Band0", PointerValue (band0),
                                                                                  "Band1", PointerValue (band1),
                                                                                  "Band2", PointerValue (band2));
  bool ok = queue->SetAttributeFailSafe ("Mode", EnumValue (PfifoFastQueueDisc::QUEUE_MODE_DSCP));
  NS_TEST_ASSERT_MSG_EQ (ok, true, "unable to set attribute");

  // Add two packets per each band
  AddPacket (queue, Ipv4Header::DSCP_AF42); // 0
  AddPacket (queue, Ipv4Header::DSCP_AF42); // 0
  AddPacket (queue, Ipv4Header::DSCP_AF13); // 1
  AddPacket (queue, Ipv4Header::DSCP_AF13); // 1
  AddPacket (queue, Ipv4Header::DSCP_AF11); // 2
  AddPacket (queue, Ipv4Header::DSCP_AF11); // 2
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (0), 2, "unexpected queue depth");
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (1), 2, "unexpected queue depth");
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (2), 2, "unexpected queue depth");
  NS_TEST_ASSERT_MSG_EQ (queue->QueueDisc::GetNPackets (), 6, "unexpected queue depth");
  // Add a third packet to each band
  AddPacket (queue, Ipv4Header::DSCP_AF42); // 0
  AddPacket (queue, Ipv4Header::DSCP_AF13); // 1
  AddPacket (queue, Ipv4Header::DSCP_AF11); // 2
  // Bands should still have two packets each
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (0), 2, "unexpected queue depth");
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (1), 2, "unexpected queue depth");
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (2), 2, "unexpected queue depth");
  NS_TEST_ASSERT_MSG_EQ (queue->QueueDisc::GetNPackets (), 6, "unexpected queue depth");
}

/**
 * This class tests that non-IP packets are handled by placing them into
 * band 1
 */
class PfifoFastQueueDiscNonIpHeader : public TestCase
{
public:
  PfifoFastQueueDiscNonIpHeader ();
  virtual ~PfifoFastQueueDiscNonIpHeader ();

private:
  virtual void DoRun (void);
};

PfifoFastQueueDiscNonIpHeader::PfifoFastQueueDiscNonIpHeader ()
  : TestCase ("Test queue with non IP header")
{
}

PfifoFastQueueDiscNonIpHeader::~PfifoFastQueueDiscNonIpHeader ()
{
}

void
PfifoFastQueueDiscNonIpHeader::DoRun (void)
{
  // all packets with non-IP headers should enqueue in band 1
  Ptr<PfifoFastQueueDisc> queue = CreateObject<PfifoFastQueueDisc> ();
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (1), 0, "unexpected queue depth");
  Ptr<Packet> p;
  p = Create<Packet> ();
  Ptr<QueueDiscItem> item;
  Ipv6Header ipv6Header;
  Address dest;
  item = Create<QueueDiscItem> (p, ipv6Header, dest, 0, 0);
  queue->Enqueue (item);
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (1), 1, "unexpected queue depth");
  p = Create<Packet> (reinterpret_cast<const uint8_t*> ("hello, world"), 12);
  item = Create<QueueDiscItem> (p, ipv6Header, dest, 0, 0);
  queue->Enqueue (item);
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (1), 2, "unexpected queue depth");
  p = Create<Packet> (100);
  uint8_t *buf = new uint8_t[100];
  uint8_t counter = 0;
  for (uint32_t i = 0; i < 100; i++)
    {
      buf[i] = counter++;
    }
  p->CopyData (buf, 100);
  item = Create<QueueDiscItem> (p, ipv6Header, dest, 0, 0);
  queue->Enqueue (item);
  NS_TEST_ASSERT_MSG_EQ (queue->GetNPackets (1), 3, "unexpected queue depth");
  delete[] buf;
}

class PfifoFastQueueDiscTestSuite : public TestSuite
{
public:
  PfifoFastQueueDiscTestSuite ();
};

PfifoFastQueueDiscTestSuite::PfifoFastQueueDiscTestSuite ()
  : TestSuite ("pfifo-fast-queue-disc", UNIT)
{
  AddTestCase (new PfifoFastQueueDiscTosPrioritization, TestCase::QUICK);
  AddTestCase (new PfifoFastQueueDiscDscpPrioritization, TestCase::QUICK);
  AddTestCase (new PfifoFastQueueDiscOverflow, TestCase::QUICK);
  AddTestCase (new PfifoFastQueueDiscNonIpHeader, TestCase::QUICK);
}

static PfifoFastQueueDiscTestSuite pfifoFastQueueTestSuite;
