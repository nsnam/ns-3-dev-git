/*
 * Copyright (c) 2015 Natale Patriciello <natale.patriciello@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */
#include "tcp-general-test.h"

#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/simple-channel.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpNewRenoCongAvoidTest");

/**
 * @ingroup internet-test
 *
 * @brief Test the behavior of RFC congestion avoidance
 *
 * From RFC 5681:\n
 *
 *     cwnd += min (N, SMSS)                      (2)
 *
 *  During congestion avoidance, cwnd is incremented by roughly 1 full-
 *  sized segment per round-trip time (RTT).  Congestion avoidance
 *  continues until congestion is detected.  The basic guidelines for
 *  incrementing cwnd during congestion avoidance are:
 *
 *     * MAY increment cwnd by SMSS bytes
 *
 *     * SHOULD increment cwnd per equation (2) once per RTT
 *
 *     * MUST NOT increment cwnd by more than SMSS bytes
 *
 * To test this behavior and these points, a tracing callback is attached
 * to the cWnd. Each time it increases, the increment is saved. Meanwhile, a
 * timer checks if an amount of time equals to the RTT has passed, and if yes,
 * it checks that the increment has not passed the 1 MSS limit.
 */
class TcpNewRenoCongAvoidNormalTest : public TcpGeneralTest
{
  public:
    /**
     * @brief Constructor.
     * @param segmentSize Segment size.
     * @param packetSize Size of the packets.
     * @param packets Number of packets.
     * @param congControl Type of congestion control.
     * @param desc The test description.
     */
    TcpNewRenoCongAvoidNormalTest(uint32_t segmentSize,
                                  uint32_t packetSize,
                                  uint32_t packets,
                                  const TypeId& congControl,
                                  const std::string& desc);

  protected:
    void CWndTrace(uint32_t oldValue, uint32_t newValue) override;
    void QueueDrop(SocketWho who) override;
    void PhyDrop(SocketWho who) override;
    void NormalClose(SocketWho who) override;
    /**
     * @brief Called each RTT (1.0 sec in the testing environment) and check
     * that the overall increment in this RTT is less or equal than 1 MSS
     */
    void Check();

    void ConfigureEnvironment() override;
    void ConfigureProperties() override;

  private:
    uint32_t m_segmentSize; //!< Segment size.
    uint32_t m_packetSize;  //!< Size of the packets.
    uint32_t m_packets;     //!< Number of packets.
    uint32_t m_increment;   //!< Congestion window increment.
    EventId m_event;        //!< Check event.
    bool m_initial;         //!< True on first run.
};

TcpNewRenoCongAvoidNormalTest::TcpNewRenoCongAvoidNormalTest(uint32_t segmentSize,
                                                             uint32_t packetSize,
                                                             uint32_t packets,
                                                             const TypeId& typeId,
                                                             const std::string& desc)
    : TcpGeneralTest(desc),
      m_segmentSize(segmentSize),
      m_packetSize(packetSize),
      m_packets(packets),
      m_increment(0),
      m_initial(true)
{
    m_congControlTypeId = typeId;
}

void
TcpNewRenoCongAvoidNormalTest::ConfigureEnvironment()
{
    TcpGeneralTest::ConfigureEnvironment();
    SetAppPktSize(m_packetSize);
    SetAppPktCount(m_packets);
    SetMTU(1500);
}

void
TcpNewRenoCongAvoidNormalTest::ConfigureProperties()
{
    TcpGeneralTest::ConfigureProperties();
    SetSegmentSize(SENDER, m_segmentSize);
    SetInitialSsThresh(SENDER, 0);
}

void
TcpNewRenoCongAvoidNormalTest::CWndTrace(uint32_t oldValue, uint32_t newValue)
{
    if (m_initial)
    {
        m_initial = false;
        return;
    }

    if (!m_event.IsPending())
    {
        m_event = Simulator::Schedule(Seconds(1), &TcpNewRenoCongAvoidNormalTest::Check, this);
    }

    m_increment += newValue - oldValue;
}

void
TcpNewRenoCongAvoidNormalTest::QueueDrop(SocketWho who)
{
    NS_FATAL_ERROR("Drop on the queue; cannot validate congestion avoidance");
}

void
TcpNewRenoCongAvoidNormalTest::PhyDrop(SocketWho who)
{
    NS_FATAL_ERROR("Drop on the phy: cannot validate congestion avoidance");
}

void
TcpNewRenoCongAvoidNormalTest::Check()
{
    uint32_t segSize = GetSegSize(TcpGeneralTest::SENDER);

    if (m_increment != 0)
    {
        NS_TEST_ASSERT_MSG_LT_OR_EQ(m_increment,
                                    segSize,
                                    "Increment exceeded segment size in one RTT");
    }

    m_increment = 0;

    m_event = Simulator::Schedule(Seconds(1), &TcpNewRenoCongAvoidNormalTest::Check, this);
}

void
TcpNewRenoCongAvoidNormalTest::NormalClose(SocketWho who)
{
    if (who == SENDER)
    {
        m_event.Cancel();
    }
}

/**
 * @ingroup internet-test
 *
 * @brief TestSuite for the behavior of RFC congestion avoidance
 */
class TcpRenoCongAvoidTestSuite : public TestSuite
{
  public:
    TcpRenoCongAvoidTestSuite()
        : TestSuite("tcp-cong-avoid-test", Type::UNIT)
    {
        std::list<TypeId> types = {
            TcpNewReno::GetTypeId(),
        };

        for (const auto& t : types)
        {
            std::string typeName = t.GetName();

            for (uint32_t i = 10; i <= 50; i += 10)
            {
                AddTestCase(new TcpNewRenoCongAvoidNormalTest(500,
                                                              500,
                                                              i,
                                                              t,
                                                              "cong avoid MSS=500, pkt_size=500," +
                                                                  typeName),
                            TestCase::Duration::QUICK);
                AddTestCase(new TcpNewRenoCongAvoidNormalTest(500,
                                                              1000,
                                                              i,
                                                              t,
                                                              "cong avoid MSS=500, pkt_size=1000," +
                                                                  typeName),
                            TestCase::Duration::QUICK);
            }
        }
    }
};

static TcpRenoCongAvoidTestSuite
    g_tcpCongAvoidNormalTest; //!< Static variable for test initialization
