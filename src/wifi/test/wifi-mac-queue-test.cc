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

#include <algorithm>

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
 * \brief Test extraction of expired MPDUs from MAC queue container
 *
 * This test verifies the correctness of the WifiMacQueueContainer methods
 * (ExtractExpiredMpdus and ExtractAllExpiredMpdus) that extract MPDUs with
 * expired lifetime from the MAC queue container.
 */
class WifiExtractExpiredMpdusTest : public TestCase
{
  public:
    WifiExtractExpiredMpdusTest();

  private:
    void DoRun() override;

    /**
     * Enqueue a new MPDU into the container.
     *
     * \param rxAddr Receiver Address of the MPDU
     * \param inflight whether the MPDU is inflight
     * \param expiryTime the expity time for the MPDU
     */
    void Enqueue(Mac48Address rxAddr, bool inflight, Time expiryTime);

    WifiMacQueueContainer m_container; //!< MAC queue container
    uint16_t m_currentSeqNo{0};        //!< sequence number of current MPDU
    Mac48Address m_txAddr;             //!< Transmitter Address of MPDUs
};

WifiExtractExpiredMpdusTest::WifiExtractExpiredMpdusTest()
    : TestCase("Test extraction of expired MPDUs from MAC queue container")
{
}

void
WifiExtractExpiredMpdusTest::Enqueue(Mac48Address rxAddr, bool inflight, Time expiryTime)
{
    WifiMacHeader header(WIFI_MAC_QOSDATA);
    header.SetAddr1(rxAddr);
    header.SetAddr2(m_txAddr);
    header.SetQosTid(0);
    header.SetSequenceNumber(m_currentSeqNo++);
    auto mpdu = Create<WifiMpdu>(Create<Packet>(), header);

    auto queueId = WifiMacQueueContainer::GetQueueId(mpdu);
    auto elemIt = m_container.insert(m_container.GetQueue(queueId).cend(), mpdu);
    elemIt->expiryTime = expiryTime;
    if (inflight)
    {
        elemIt->inflights.emplace(0, mpdu);
    }
    elemIt->deleter = [](auto mpdu) {};
}

void
WifiExtractExpiredMpdusTest::DoRun()
{
    m_txAddr = Mac48Address::Allocate();
    auto rxAddr1 = Mac48Address::Allocate();
    auto rxAddr2 = Mac48Address::Allocate();

    /**
     * At simulation time 25ms:
     *
     * Container queue for rxAddr1
     * ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
     * │Exp│Exp│Exp│Exp│   │   │   │   │   │   │   │
     * │Inf│   │Inf│   │Inf│   │Inf│   │   │   │   │
     * │ 0 │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │ 8 │ 9 │10 │
     * └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
     *
     * Container queue for rxAddr2
     * ┌───┬───┬───┬───┬───┬───┬───┬───┬───┐
     * │Exp│Exp│Exp│   │   │   │   │   │   │
     * │   │Inf│Inf│   │Inf│Inf│   │   │   │
     * │11 │12 │13 │14 │15 │16 │17 │18 │19 │
     * └───┴───┴───┴───┴───┴───┴───┴───┴───┘
     */
    Enqueue(rxAddr1, true, MilliSeconds(10));
    Enqueue(rxAddr1, false, MilliSeconds(10));
    Enqueue(rxAddr1, true, MilliSeconds(12));
    Enqueue(rxAddr1, false, MilliSeconds(15));
    Enqueue(rxAddr1, true, MilliSeconds(30));
    Enqueue(rxAddr1, false, MilliSeconds(30));
    Enqueue(rxAddr1, true, MilliSeconds(35));
    Enqueue(rxAddr1, false, MilliSeconds(35));
    Enqueue(rxAddr1, false, MilliSeconds(40));
    Enqueue(rxAddr1, false, MilliSeconds(75));
    Enqueue(rxAddr1, false, MilliSeconds(75));

    Enqueue(rxAddr2, false, MilliSeconds(11));
    Enqueue(rxAddr2, true, MilliSeconds(11));
    Enqueue(rxAddr2, true, MilliSeconds(13));
    Enqueue(rxAddr2, false, MilliSeconds(30));
    Enqueue(rxAddr2, true, MilliSeconds(35));
    Enqueue(rxAddr2, true, MilliSeconds(40));
    Enqueue(rxAddr2, false, MilliSeconds(40));
    Enqueue(rxAddr2, false, MilliSeconds(70));
    Enqueue(rxAddr2, false, MilliSeconds(75));

    WifiContainerQueueId queueId1{WIFI_QOSDATA_QUEUE, WIFI_UNICAST, rxAddr1, 0};
    WifiContainerQueueId queueId2{WIFI_QOSDATA_QUEUE, WIFI_UNICAST, rxAddr2, 0};

    Simulator::Schedule(MilliSeconds(25), [&]() {
        /**
         * Extract expired MPDUs from container queue 1
         */
        auto [first1, last1] = m_container.ExtractExpiredMpdus(queueId1);
        // MPDU 0 not extracted because inflight, MPDU 1 extracted
        NS_TEST_EXPECT_MSG_EQ((first1 != last1), true, "Expected one MPDU extracted");
        NS_TEST_EXPECT_MSG_EQ(first1->mpdu->GetHeader().GetSequenceNumber(),
                              1,
                              "Unexpected extracted MPDU");
        first1++;
        // MPDU 2 not extracted because inflight, MPDU 3 extracted
        NS_TEST_EXPECT_MSG_EQ((first1 != last1), true, "Expected two MPDUs extracted");
        NS_TEST_EXPECT_MSG_EQ(first1->mpdu->GetHeader().GetSequenceNumber(),
                              3,
                              "Unexpected extracted MPDU");
        first1++;
        // No other expired MPDU
        NS_TEST_EXPECT_MSG_EQ((first1 == last1), true, "Did not expect other expired MPDUs");

        // If we try to extract expired MPDUs again, the returned set is empty
        {
            auto [first, last] = m_container.ExtractExpiredMpdus(queueId1);
            NS_TEST_EXPECT_MSG_EQ((first == last), true, "Did not expect other expired MPDUs");
        }

        /**
         * Extract expired MPDUs from container queue 2
         */
        auto [first2, last2] = m_container.ExtractExpiredMpdus(queueId2);
        // MPDU 11 extracted
        NS_TEST_EXPECT_MSG_EQ((first2 != last2), true, "Expected one MPDU extracted");
        NS_TEST_EXPECT_MSG_EQ(first2->mpdu->GetHeader().GetSequenceNumber(),
                              11,
                              "Unexpected extracted MPDU");
        first2++;
        // MPDU 12 and 13 not extracted because inflight, no other expired MPDU
        NS_TEST_EXPECT_MSG_EQ((first2 == last2), true, "Did not expect other expired MPDUs");

        // If we try to extract expired MPDUs again, the returned set is empty
        {
            auto [first, last] = m_container.ExtractExpiredMpdus(queueId2);
            NS_TEST_EXPECT_MSG_EQ((first == last), true, "Did not expect other expired MPDUs");
        }
    });

    /**
     * At simulation time 50ms:
     *
     * Container queue for rxAddr1
     * ┌───┬───┬───┬───┬───┬───┬───┬───┬───┐
     * │Exp│Exp│Exp│Exp│Exp│Exp│Exp│   │   │
     * │Inf│Inf│Inf│   │Inf│   │   │   │   │
     * │ 0 │ 2 │ 4 │ 5 │ 6 │ 7 │ 8 │ 9 │10 │
     * └───┴───┴───┴───┴───┴───┴───┴───┴───┘
     *
     * Container queue for rxAddr2
     * ┌───┬───┬───┬───┬───┬───┬───┬───┐
     * │Exp│Exp│Exp│Exp│Exp│Exp│   │   │
     * │Inf│Inf│   │Inf│Inf│   │   │   │
     * │12 │13 │14 │15 │16 │17 │18 │19 │
     * └───┴───┴───┴───┴───┴───┴───┴───┘
     */
    Simulator::Schedule(MilliSeconds(50), [&]() {
        /**
         * Extract all expired MPDUs (from container queue 1 and 2)
         */
        auto [first, last] = m_container.ExtractAllExpiredMpdus();

        std::set<uint16_t> expectedSeqNo{5, 7, 8, 14, 17};
        std::set<uint16_t> actualSeqNo;

        std::transform(first, last, std::inserter(actualSeqNo, actualSeqNo.end()), [](auto& elem) {
            return elem.mpdu->GetHeader().GetSequenceNumber();
        });

        NS_TEST_EXPECT_MSG_EQ(expectedSeqNo.size(),
                              actualSeqNo.size(),
                              "Unexpected number of MPDUs extracted");

        for (auto expectedIt = expectedSeqNo.begin(), actualIt = actualSeqNo.begin();
             expectedIt != expectedSeqNo.end();
             ++expectedIt, ++actualIt)
        {
            NS_TEST_EXPECT_MSG_EQ(*expectedIt, *actualIt, "Unexpected extracted MPDU");
        }

        // If we try to extract expired MPDUs again, the returned set is empty
        {
            auto [first, last] = m_container.ExtractAllExpiredMpdus();
            NS_TEST_EXPECT_MSG_EQ((first == last), true, "Did not expect other expired MPDUs");
        }

        /**
         * Check MPDUs remaining in container queue 1
         */
        auto elemIt = m_container.GetQueue(queueId1).begin();
        auto endIt = m_container.GetQueue(queueId1).end();
        NS_TEST_EXPECT_MSG_EQ((elemIt != endIt),
                              true,
                              "There should be other MPDU(s) in container queue 1");
        NS_TEST_EXPECT_MSG_EQ(elemIt->mpdu->GetHeader().GetSequenceNumber(),
                              0,
                              "Unexpected queued MPDU");
        elemIt++;
        NS_TEST_EXPECT_MSG_EQ((elemIt != endIt),
                              true,
                              "There should be other MPDU(s) in container queue 1");
        NS_TEST_EXPECT_MSG_EQ(elemIt->mpdu->GetHeader().GetSequenceNumber(),
                              2,
                              "Unexpected queued MPDU");
        elemIt++;
        NS_TEST_EXPECT_MSG_EQ((elemIt != endIt),
                              true,
                              "There should be other MPDU(s) in container queue 1");
        NS_TEST_EXPECT_MSG_EQ(elemIt->mpdu->GetHeader().GetSequenceNumber(),
                              4,
                              "Unexpected queued MPDU");
        elemIt++;
        NS_TEST_EXPECT_MSG_EQ((elemIt != endIt),
                              true,
                              "There should be other MPDU(s) in container queue 1");
        NS_TEST_EXPECT_MSG_EQ(elemIt->mpdu->GetHeader().GetSequenceNumber(),
                              6,
                              "Unexpected queued MPDU");
        elemIt++;
        NS_TEST_EXPECT_MSG_EQ((elemIt != endIt),
                              true,
                              "There should be other MPDU(s) in container queue 1");
        NS_TEST_EXPECT_MSG_EQ(elemIt->mpdu->GetHeader().GetSequenceNumber(),
                              9,
                              "Unexpected queued MPDU");
        elemIt++;
        NS_TEST_EXPECT_MSG_EQ((elemIt != endIt),
                              true,
                              "There should be other MPDU(s) in container queue 1");
        NS_TEST_EXPECT_MSG_EQ(elemIt->mpdu->GetHeader().GetSequenceNumber(),
                              10,
                              "Unexpected queued MPDU");
        elemIt++;
        NS_TEST_EXPECT_MSG_EQ((elemIt == endIt),
                              true,
                              "There should be no other MPDU in container queue 1");

        /**
         * Check MPDUs remaining in container queue 2
         */
        elemIt = m_container.GetQueue(queueId2).begin();
        endIt = m_container.GetQueue(queueId2).end();
        NS_TEST_EXPECT_MSG_EQ((elemIt != endIt),
                              true,
                              "There should be other MPDU(s) in container queue 2");
        NS_TEST_EXPECT_MSG_EQ(elemIt->mpdu->GetHeader().GetSequenceNumber(),
                              12,
                              "Unexpected queued MPDU");
        elemIt++;
        NS_TEST_EXPECT_MSG_EQ((elemIt != endIt),
                              true,
                              "There should be other MPDU(s) in container queue 2");
        NS_TEST_EXPECT_MSG_EQ(elemIt->mpdu->GetHeader().GetSequenceNumber(),
                              13,
                              "Unexpected queued MPDU");
        elemIt++;
        NS_TEST_EXPECT_MSG_EQ((elemIt != endIt),
                              true,
                              "There should be other MPDU(s) in container queue 2");
        NS_TEST_EXPECT_MSG_EQ(elemIt->mpdu->GetHeader().GetSequenceNumber(),
                              15,
                              "Unexpected queued MPDU");
        elemIt++;
        NS_TEST_EXPECT_MSG_EQ((elemIt != endIt),
                              true,
                              "There should be other MPDU(s) in container queue 2");
        NS_TEST_EXPECT_MSG_EQ(elemIt->mpdu->GetHeader().GetSequenceNumber(),
                              16,
                              "Unexpected queued MPDU");
        elemIt++;
        NS_TEST_EXPECT_MSG_EQ((elemIt != endIt),
                              true,
                              "There should be other MPDU(s) in container queue 2");
        NS_TEST_EXPECT_MSG_EQ(elemIt->mpdu->GetHeader().GetSequenceNumber(),
                              18,
                              "Unexpected queued MPDU");
        elemIt++;
        NS_TEST_EXPECT_MSG_EQ((elemIt != endIt),
                              true,
                              "There should be other MPDU(s) in container queue 2");
        NS_TEST_EXPECT_MSG_EQ(elemIt->mpdu->GetHeader().GetSequenceNumber(),
                              19,
                              "Unexpected queued MPDU");
        elemIt++;
        NS_TEST_EXPECT_MSG_EQ((elemIt == endIt),
                              true,
                              "There should be no other MPDU in container queue 2");
    });

    Simulator::Run();
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
    : TestSuite("wifi-mac-queue", Type::UNIT)
{
    AddTestCase(new WifiMacQueueDropOldestTest, TestCase::Duration::QUICK);
    AddTestCase(new WifiExtractExpiredMpdusTest, TestCase::Duration::QUICK);
}

static WifiMacQueueTestSuite g_wifiMacQueueTestSuite; ///< the test suite
