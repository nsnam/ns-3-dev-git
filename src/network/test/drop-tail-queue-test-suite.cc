/*
 * Copyright (c) 2007 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/drop-tail-queue.h"
#include "ns3/string.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * DropTailQueue unit tests.
 */
class DropTailQueueTestCase : public TestCase
{
  public:
    DropTailQueueTestCase();
    void DoRun() override;
};

DropTailQueueTestCase::DropTailQueueTestCase()
    : TestCase("Sanity check on the drop tail queue implementation")
{
}

void
DropTailQueueTestCase::DoRun()
{
    Ptr<DropTailQueue<Packet>> queue = CreateObject<DropTailQueue<Packet>>();
    NS_TEST_EXPECT_MSG_EQ(queue->SetAttributeFailSafe("MaxSize", StringValue("3p")),
                          true,
                          "Verify that we can actually set the attribute");

    Ptr<Packet> p1;
    Ptr<Packet> p2;
    Ptr<Packet> p3;
    Ptr<Packet> p4;
    p1 = Create<Packet>();
    p2 = Create<Packet>();
    p3 = Create<Packet>();
    p4 = Create<Packet>();

    NS_TEST_EXPECT_MSG_EQ(queue->GetNPackets(), 0, "There should be no packets in there");
    queue->Enqueue(p1);
    NS_TEST_EXPECT_MSG_EQ(queue->GetNPackets(), 1, "There should be one packet in there");
    queue->Enqueue(p2);
    NS_TEST_EXPECT_MSG_EQ(queue->GetNPackets(), 2, "There should be two packets in there");
    queue->Enqueue(p3);
    NS_TEST_EXPECT_MSG_EQ(queue->GetNPackets(), 3, "There should be three packets in there");
    queue->Enqueue(p4); // will be dropped
    NS_TEST_EXPECT_MSG_EQ(queue->GetNPackets(), 3, "There should be still three packets in there");

    Ptr<Packet> packet;

    packet = queue->Dequeue();
    NS_TEST_EXPECT_MSG_NE(packet, nullptr, "I want to remove the first packet");
    NS_TEST_EXPECT_MSG_EQ(queue->GetNPackets(), 2, "There should be two packets in there");
    NS_TEST_EXPECT_MSG_EQ(packet->GetUid(), p1->GetUid(), "was this the first packet ?");

    packet = queue->Dequeue();
    NS_TEST_EXPECT_MSG_NE(packet, nullptr, "I want to remove the second packet");
    NS_TEST_EXPECT_MSG_EQ(queue->GetNPackets(), 1, "There should be one packet in there");
    NS_TEST_EXPECT_MSG_EQ(packet->GetUid(), p2->GetUid(), "Was this the second packet ?");

    packet = queue->Dequeue();
    NS_TEST_EXPECT_MSG_NE(packet, nullptr, "I want to remove the third packet");
    NS_TEST_EXPECT_MSG_EQ(queue->GetNPackets(), 0, "There should be no packets in there");
    NS_TEST_EXPECT_MSG_EQ(packet->GetUid(), p3->GetUid(), "Was this the third packet ?");

    packet = queue->Dequeue();
    NS_TEST_EXPECT_MSG_EQ(packet, nullptr, "There are really no packets in there");
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief DropTail Queue TestSuite
 */
class DropTailQueueTestSuite : public TestSuite
{
  public:
    DropTailQueueTestSuite()
        : TestSuite("drop-tail-queue", Type::UNIT)
    {
        AddTestCase(new DropTailQueueTestCase(), TestCase::Duration::QUICK);
    }
};

static DropTailQueueTestSuite g_dropTailQueueTestSuite; //!< Static variable for test initialization
