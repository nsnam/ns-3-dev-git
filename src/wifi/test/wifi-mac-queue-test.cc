/*
 * Copyright (c) 2021 IITP RAS
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
 * Author: Alexander Krotov <krotov@iitp.ru>
 */

#include "ns3/fcfs-wifi-queue-scheduler.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/wifi-mac-queue.h"

using namespace ns3;

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test DROP_OLDEST setting.
 *
 * This test verifies the correctness of DROP_OLDEST policy when packets
 * are pushed into the front of the queue. This case is not handled
 * by the underlying ns3::Queue<WifiMpdu>.
 */
class WifiMacQueueDropOldestTest : public TestCase
{
  public:
    /**
     * \brief Constructor
     */
    WifiMacQueueDropOldestTest();

    void DoRun() override;
};

WifiMacQueueDropOldestTest::WifiMacQueueDropOldestTest()
    : TestCase("Test DROP_OLDEST setting")
{
}

void
WifiMacQueueDropOldestTest::DoRun()
{
    auto wifiMacQueue = CreateObject<WifiMacQueue>(AC_BE);
    wifiMacQueue->SetMaxSize(QueueSize("5p"));
    auto wifiMacScheduler = CreateObject<FcfsWifiQueueScheduler>();
    wifiMacScheduler->SetAttribute("DropPolicy", EnumValue(FcfsWifiQueueScheduler::DROP_OLDEST));
    wifiMacScheduler->m_perAcInfo[AC_BE].wifiMacQueue = wifiMacQueue;
    wifiMacQueue->SetScheduler(wifiMacScheduler);

    Mac48Address addr1 = Mac48Address::Allocate();

    // Initialize the queue with 5 packets.
    std::list<uint64_t> packetUids;
    for (uint32_t i = 0; i < 5; i++)
    {
        WifiMacHeader header;
        header.SetType(WIFI_MAC_QOSDATA);
        header.SetAddr1(addr1);
        header.SetQosTid(0);
        auto packet = Create<Packet>();
        auto item = Create<WifiMpdu>(packet, header);
        wifiMacQueue->Enqueue(item);

        packetUids.push_back(packet->GetUid());
    }

    // Check that all elements are inserted successfully.
    auto mpdu = wifiMacQueue->PeekByTidAndAddress(0, addr1);
    NS_TEST_EXPECT_MSG_EQ(wifiMacQueue->GetNPackets(),
                          5,
                          "Queue has unexpected number of elements");
    for (auto packetUid : packetUids)
    {
        NS_TEST_EXPECT_MSG_EQ(mpdu->GetPacket()->GetUid(),
                              packetUid,
                              "Stored packet is not the expected one");
        mpdu = wifiMacQueue->PeekByTidAndAddress(0, addr1, mpdu);
    }

    // Push another element into the queue.
    WifiMacHeader header;
    header.SetType(WIFI_MAC_QOSDATA);
    header.SetAddr1(addr1);
    header.SetQosTid(0);
    auto packet = Create<Packet>();
    auto item = Create<WifiMpdu>(packet, header);
    wifiMacQueue->Enqueue(item);

    // Update the list of expected packet UIDs.
    packetUids.pop_front();
    packetUids.push_back(packet->GetUid());

    // Check that front packet was replaced correctly.
    mpdu = wifiMacQueue->PeekByTidAndAddress(0, addr1);
    NS_TEST_EXPECT_MSG_EQ(wifiMacQueue->GetNPackets(),
                          5,
                          "Queue has unexpected number of elements");
    for (auto packetUid : packetUids)
    {
        NS_TEST_EXPECT_MSG_EQ(mpdu->GetPacket()->GetUid(),
                              packetUid,
                              "Stored packet is not the expected one");
        mpdu = wifiMacQueue->PeekByTidAndAddress(0, addr1, mpdu);
    }

    wifiMacScheduler->Dispose();
    Simulator::Destroy();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Wifi MAC Queue Test Suite
 */
class WifiMacQueueTestSuite : public TestSuite
{
  public:
    WifiMacQueueTestSuite();
};

WifiMacQueueTestSuite::WifiMacQueueTestSuite()
    : TestSuite("wifi-mac-queue", UNIT)
{
    AddTestCase(new WifiMacQueueDropOldestTest, TestCase::QUICK);
}

static WifiMacQueueTestSuite g_wifiMacQueueTestSuite; ///< the test suite
