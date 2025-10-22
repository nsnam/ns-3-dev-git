/*
 * Copyright (c) 2025 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: N Nagabhushanam    <thechosentwins2005@gmail.com>
 *          Namburi Yaswanth   <yaswanthnamburi1010@gmail.com>
 *          Vishruth S Kumar   <vishruthskumar@gmail.com>
 *          Yashas             <yashas80dj@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */
#include "ns3/log.h"
#include "ns3/tcp-cubic.h"
#include "ns3/tcp-linux-reno.h"
#include "ns3/tcp-socket-state.h"
#include "ns3/test.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("TcpAbeTestSuite");

constexpr uint32_t WITHOUT_ABE = 0; //!< Test case without ABE
constexpr uint32_t WITH_ABE = 1;    //!< Test case with ABE

/**
 * @ingroup internet-test
 * @brief Test Socket Base class for ABE testing
 *
 * This class provides access to the TCP socket state and allows
 * manipulation of ECN state for testing purposes.
 */
class TestTcpSocketBase : public TcpSocketBase
{
  public:
    /**
     * @brief Get the pointer for socket state
     * @return ptr to TcpSocketState
     */
    Ptr<TcpSocketState> GetTcb() const
    {
        return m_tcb;
    }

    /**
     * @brief Set the ECE state for testing ECN behavior
     */
    void SetECE()
    {
        m_tcb->m_ecnState = TcpSocketState::ECN_ECE_RCVD;
    }
};

/**
 * @ingroup internet-test
 * @brief Checks if users are able to enable ABE or not.
 **/

class TcpAbeToggleTest : public TestCase
{
  protected:
    Ptr<TestTcpSocketBase> m_socket; //!< Socket used for testing

  public:
    /**
     * @brief Constructor
     * @param desc Test case description
     */
    TcpAbeToggleTest(const std::string& desc)
        : TestCase(desc)
    {
    }

    /**
     * @brief Runs the test
     */
    void DoRun() override
    {
        NS_LOG_FUNCTION(this);
        m_socket = CreateObject<TestTcpSocketBase>();
        m_socket->SetUseAbe(true);
        NS_TEST_EXPECT_MSG_EQ(m_socket->GetTcb()->m_abeEnabled, true, "ABE should be enabled");
        NS_TEST_EXPECT_MSG_EQ(m_socket->GetTcb()->m_useEcn,
                              true,
                              "ECN should be enabled along with ABE");

        m_socket->SetUseAbe(false);
        NS_TEST_EXPECT_MSG_EQ(m_socket->GetTcb()->m_abeEnabled, false, "ABE should be disabled");
    }
};

/**
 * @ingroup internet-test
 * @brief Test case for congestion control algorithms with ABE
 *
 * This test verifies that when a TCP socket has ABE enabled, the congestion
 * window value returned by its GetSsThresh() method will be calculated using
 * the default BetaEcn value, and when ABE is disabled, the standard
 * multiplicative decrease by half is used.
 * ABE should only work for TcpCubic, TcpNewReno, TcpLinuxReno as specified in
 * RFC-8511
 */
class TcpAbeTest : public TestCase
{
  private:
    uint32_t testCase;        //!< Test case number
    uint32_t m_segmentSize;   //!< Segment size
    uint32_t m_initialCwnd;   //!< Initial congestion window
    uint32_t m_expectedCwnd;  //!< Expected congestion window after applying Beta(BetaLoss)/BetaEcn
    uint32_t m_bytesInFlight; //!< Bytes in flight
    TypeId m_congestionControlType;  //!< TypeId of the congestion control algorithm type used for
                                     //!< the test
    Ptr<TestTcpSocketBase> m_socket; //!< Socket used for testing

  public:
    /**
     * @brief Constructor
     * @param desc Test case description
     * @param testCase Test case identifier (0 for without ABE, 1 for with ABE)
     * @param segmentSize Segment size
     * @param initialCwnd Initial congestion window
     * @param expectedCwnd Expected congestion window after applying Beta(BetaLoss)/BetaEcn
     * @param bytesInFlight Bytes in flight
     * @param congestionControlType TypeId of the congestion control algorithm type used for the
     * test
     */
    TcpAbeTest(uint32_t testCase,
               uint32_t segmentSize,
               uint32_t initialCwnd,
               uint32_t expectedCwnd,
               uint32_t bytesInFlight,
               TypeId& congestionControlType,
               const std::string& desc)
        : TestCase(desc),
          testCase(testCase),
          m_segmentSize(segmentSize),
          m_initialCwnd(initialCwnd),
          m_expectedCwnd(expectedCwnd),
          m_bytesInFlight(bytesInFlight),
          m_congestionControlType(congestionControlType)
    {
        NS_LOG_FUNCTION(this << desc);
    }

    /**
     * @brief Runs the test
     */
    void DoRun() override
    {
        NS_LOG_FUNCTION(this);
        m_socket = CreateObject<TestTcpSocketBase>();
        ObjectFactory congestionAlgorithmFactory;
        congestionAlgorithmFactory.SetTypeId(m_congestionControlType);
        Ptr<TcpCongestionOps> algo = congestionAlgorithmFactory.Create<TcpCongestionOps>();
        m_socket->SetCongestionControlAlgorithm(algo);

        if (testCase == WITH_ABE)
        {
            m_socket->SetUseAbe(true);
            m_socket->SetECE();
            m_socket->GetTcb()->m_segmentSize = m_segmentSize;
            m_socket->GetTcb()->m_cWnd = m_initialCwnd;
            m_socket->GetTcb()->m_bytesInFlight = m_bytesInFlight;
            uint32_t newCwnd = algo->GetSsThresh(m_socket->GetTcb(), m_bytesInFlight);
            NS_TEST_EXPECT_MSG_EQ(
                newCwnd,
                m_expectedCwnd,
                m_congestionControlType.GetName() +
                    " congestion control with ABE should apply BetaEcn correctly");
        }
        else if (testCase == WITHOUT_ABE)
        {
            m_socket->SetUseAbe(false);
            m_socket->SetECE();
            m_socket->GetTcb()->m_segmentSize = m_segmentSize;
            m_socket->GetTcb()->m_cWnd = m_initialCwnd;
            m_socket->GetTcb()->m_bytesInFlight = m_bytesInFlight;
            uint32_t newCwnd = algo->GetSsThresh(m_socket->GetTcb(), m_bytesInFlight);
            NS_TEST_EXPECT_MSG_EQ(
                newCwnd,
                m_expectedCwnd,
                m_congestionControlType.GetName() +
                    " congestion control without ABE should apply Beta correctly");
        }
        else
        {
            NS_TEST_EXPECT_MSG_EQ(0, 1, "Invalid test case");
        }
        Simulator::Destroy();
        m_socket = nullptr;
    }
};

/**
 * @ingroup internet-test
 * @brief Test suite for Alternative Backoff with ECN (ABE)
 *
 * This test suite verifies the behavior of TCP ABE with different
 * congestion control algorithms.
 */
class TcpAbeTestSuite : public TestSuite
{
  public:
    /**
     * @brief Constructor
     */
    TcpAbeTestSuite()
        : TestSuite("tcp-abe-test", Type::UNIT)
    {
        AddTestCase(new TcpAbeToggleTest("Test enabling and disabling ABE"),
                    TestCase::Duration::QUICK);

        TypeId cong_control_type = TcpCubic::GetTypeId();
        AddTestCase(new TcpAbeTest(WITHOUT_ABE,
                                   1,
                                   1000,
                                   700,
                                   100,
                                   cong_control_type,
                                   "Test TCP CUBIC without ABE"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpAbeTest(WITH_ABE,
                                   1,
                                   1000,
                                   850,
                                   100,
                                   cong_control_type,
                                   "Test TCP CUBIC with ABE"),
                    TestCase::Duration::QUICK);

        cong_control_type = TcpLinuxReno::GetTypeId();
        AddTestCase(new TcpAbeTest(WITHOUT_ABE,
                                   1,
                                   1000,
                                   500,
                                   100,
                                   cong_control_type,
                                   "Test TCP Linux Reno without ABE"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpAbeTest(WITH_ABE,
                                   1,
                                   1000,
                                   700,
                                   100,
                                   cong_control_type,
                                   "Test TCP Linux Reno with ABE"),
                    TestCase::Duration::QUICK);

        cong_control_type = TcpNewReno::GetTypeId();
        AddTestCase(new TcpAbeTest(WITHOUT_ABE,
                                   1,
                                   1000,
                                   500,
                                   1000,
                                   cong_control_type,
                                   "Test TCP New Reno without ABE"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpAbeTest(WITH_ABE,
                                   1,
                                   1000,
                                   700,
                                   1000,
                                   cong_control_type,
                                   "Test TCP New Reno with ABE"),
                    TestCase::Duration::QUICK);
    }
};

static TcpAbeTestSuite g_tcpAbeTestSuite; //!< static var for test initialization

} // namespace ns3
