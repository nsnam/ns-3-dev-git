/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 University of Washington
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

#include "ns3/test.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/uinteger.h"

using namespace ns3;

class DropTailQueueTestCase : public TestCase
{
public:
  DropTailQueueTestCase ();
  virtual void DoRun (void);
};

DropTailQueueTestCase::DropTailQueueTestCase ()
  : TestCase ("Sanity check on the drop tail queue implementation")
{
}
void
DropTailQueueTestCase::DoRun (void)
{
  Ptr<DropTailQueue> queue = CreateObject<DropTailQueue> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxPackets", UintegerValue (3)), true,
                         "Verify that we can actually set the attribute");

  Ptr<Packet> p1, p2, p3, p4;
  p1 = Create<Packet> ();
  p2 = Create<Packet> ();
  p3 = Create<Packet> ();
  p4 = Create<Packet> ();

  NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), 0, "There should be no packets in there");
  queue->Enqueue (Create<QueueItem> (p1));
  NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), 1, "There should be one packet in there");
  queue->Enqueue (Create<QueueItem> (p2));
  NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), 2, "There should be two packets in there");
  queue->Enqueue (Create<QueueItem> (p3));
  NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), 3, "There should be three packets in there");
  queue->Enqueue (Create<QueueItem> (p4)); // will be dropped
  NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), 3, "There should be still three packets in there");

  Ptr<QueueItem> item;

  item = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "I want to remove the first packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), 2, "There should be two packets in there");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p1->GetUid (), "was this the first packet ?");

  item = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "I want to remove the second packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), 1, "There should be one packet in there");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p2->GetUid (), "Was this the second packet ?");

  item = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "I want to remove the third packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), 0, "There should be no packets in there");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p3->GetUid (), "Was this the third packet ?");

  item = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item == 0), true, "There are really no packets in there");
}

static class DropTailQueueTestSuite : public TestSuite
{
public:
  DropTailQueueTestSuite ()
    : TestSuite ("drop-tail-queue", UNIT)
  {
    AddTestCase (new DropTailQueueTestCase (), TestCase::QUICK);
  }
} g_dropTailQueueTestSuite;
