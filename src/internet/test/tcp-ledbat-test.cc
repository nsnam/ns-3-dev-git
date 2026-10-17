/*
 * Copyright (c) 2016 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ankit Deepak <adadeepak8@gmail.com>
 * Modified by: S B L Prateek <sblprateek@gmail.com>
 *
 */

#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-ledbat.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpLedbatTestSuite");

/**
 * @ingroup internet-test
 *
 * @brief LEDBAT should be same as NewReno during slow start, and when timestamps are disabled
 */
class TcpLedbatToNewReno : public TestCase
{
  public:
    /**
     * @brief Constructor
     *
     * @param cWnd congestion window
     * @param segmentSize segment size
     * @param ssThresh slow start threshold
     * @param segmentsAcked segments acked
     * @param bytesInFlight bytes in flight
     * @param rtt RTT
     * @param name Name of the test
     */
    TcpLedbatToNewReno(uint32_t cWnd,
                       uint32_t segmentSize,
                       uint32_t ssThresh,
                       uint32_t segmentsAcked,
                       uint32_t bytesInFlight,
                       Time rtt,
                       const std::string& name);

  private:
    void DoRun() override;

    uint32_t m_cWnd;             //!< cWnd
    uint32_t m_segmentSize;      //!< segment size
    uint32_t m_segmentsAcked;    //!< segments acked
    uint32_t m_ssThresh;         //!< ss thresh
    Time m_rtt;                  //!< rtt
    uint32_t m_bytesInFlight;    //!< bytes in flight
    Ptr<TcpSocketState> m_state; //!< state
};

TcpLedbatToNewReno::TcpLedbatToNewReno(uint32_t cWnd,
                                       uint32_t segmentSize,
                                       uint32_t ssThresh,
                                       uint32_t segmentsAcked,
                                       uint32_t bytesInFlight,
                                       Time rtt,
                                       const std::string& name)
    : TestCase(name),
      m_cWnd(cWnd),
      m_segmentSize(segmentSize),
      m_segmentsAcked(segmentsAcked),
      m_ssThresh(ssThresh),
      m_rtt(rtt),
      m_bytesInFlight(bytesInFlight)
{
}

void
TcpLedbatToNewReno::DoRun()
{
    m_state = CreateObject<TcpSocketState>();
    m_state->m_cWnd = m_cWnd;
    m_state->m_ssThresh = m_ssThresh;
    m_state->m_segmentSize = m_segmentSize;
    m_state->m_bytesInFlight = m_bytesInFlight;

    Ptr<TcpSocketState> state = CreateObject<TcpSocketState>();
    state->m_cWnd = m_cWnd;
    state->m_ssThresh = m_ssThresh;
    state->m_segmentSize = m_segmentSize;
    state->m_bytesInFlight = m_bytesInFlight;

    Ptr<TcpLedbat> cong = CreateObject<TcpLedbat>();
    cong->IncreaseWindow(m_state, m_segmentsAcked);

    Ptr<TcpNewReno> NewRenoCong = CreateObject<TcpNewReno>();
    NewRenoCong->IncreaseWindow(state, m_segmentsAcked);

    NS_TEST_ASSERT_MSG_EQ(m_state->m_cWnd.Get(),
                          state->m_cWnd.Get(),
                          "cWnd has not updated correctly");

    Simulator::Destroy();
}

/**
 * @ingroup internet-test
 *
 * @brief Test to validate cWnd increment/decrement in LEDBAT Congestion Avoidance Phase
 */
class TcpLedbatCongestionAvoidanceTest : public TestCase
{
  public:
    /**
     * @brief Type of expected CWND behavior in LEDBAT congestion avoidance
     */
    enum TestType
    {
        /**
         * To enable check for CWND increases because the measured queue delay is
         * below the target delay (positive offset).
         */
        INCREMENT_TEST,

        /**
         * To enable check that CWND increase is limited by the maximum allowed CWND,
         * computed from flight size before ack (bytesInFlight + segmentsAcked * segmentSize).
         */
        MAX_CWND_TEST,

        /**
         * To enable check for CWND decreases because the measured queue delay is
         * above the target delay (negative offset).
         */
        DECREMENT_TEST,

        /**
         * To enable check that CWND decrease is limited by the minimum CWND constraint
         * (MinCwnd × segmentSize).
         */
        MIN_CWND_TEST
    };

    /**
     * @brief Constructor
     *
     * @param cWnd congestion window
     * @param segmentSize segment size
     * @param ssThresh slow start threshold
     * @param segmentsAcked segments acked
     * @param bytesInFlight bytes in flight
     * @param rtt RTT
     * @param rcvTimestampValue Receive timestamp for delay estimation
     * @param currentRcvTimestampEchoReply Echo reply timestamp for current delay
     * @param testType Expected test behavior
     * @param name Name of the test
     */

    TcpLedbatCongestionAvoidanceTest(uint32_t cWnd,
                                     uint32_t segmentSize,
                                     uint32_t ssThresh,
                                     uint32_t segmentsAcked,
                                     uint32_t bytesInFlight,
                                     Time rtt,
                                     uint32_t rcvTimestampValue,
                                     uint32_t currentRcvTimestampEchoReply,
                                     TestType testType,
                                     const std::string& name);

  private:
    void DoRun() override;

    uint32_t m_cWnd;                         //!< cWnd
    uint32_t m_segmentSize;                  //!< segment size
    uint32_t m_segmentsAcked;                //!< segments acked
    uint32_t m_ssThresh;                     //!< ss thresh
    Time m_rtt;                              //!< rtt
    uint32_t m_bytesInFlight;                //!< bytes in flight (after ACK)
    uint32_t m_rcvTimestampValue;            //!< received timestamp value
    uint32_t m_currentRcvTimestampEchoReply; //!< current echoed timestamp value
    TestType m_testType;                     //!< test type
    Ptr<TcpSocketState> m_state;             //!< state
};

TcpLedbatCongestionAvoidanceTest::TcpLedbatCongestionAvoidanceTest(
    uint32_t cWnd,
    uint32_t segmentSize,
    uint32_t ssThresh,
    uint32_t segmentsAcked,
    uint32_t bytesInFlight,
    Time rtt,
    uint32_t rcvTimestampValue,
    uint32_t currentRcvTimestampEchoReply,
    TestType testType,
    const std::string& name)
    : TestCase(name),
      m_cWnd(cWnd),
      m_segmentSize(segmentSize),
      m_segmentsAcked(segmentsAcked),
      m_ssThresh(ssThresh),
      m_rtt(rtt),
      m_bytesInFlight(bytesInFlight),
      m_rcvTimestampValue(rcvTimestampValue),
      m_currentRcvTimestampEchoReply(currentRcvTimestampEchoReply),
      m_testType(testType)
{
}

void
TcpLedbatCongestionAvoidanceTest::DoRun()
{
    m_state = CreateObject<TcpSocketState>();
    m_state->m_cWnd = m_cWnd;
    m_state->m_ssThresh = m_ssThresh;
    m_state->m_segmentSize = m_segmentSize;
    m_state->m_bytesInFlight = m_bytesInFlight;

    Ptr<TcpLedbat> cong = CreateObject<TcpLedbat>();
    cong->SetAttribute("Gain", DoubleValue(1.0)); // Default LEDBAT GAIN value as per RFC 6817
    cong->SetAttribute("AllowedIncrease",
                       DoubleValue(1.0)); // Default LEDBAT Allowed Increase value as per RFC 6817
    cong->SetAttribute("SSParam", StringValue("no"));
    cong->SetAttribute("NoiseFilterLen", UintegerValue(1));
    cong->SetAttribute("MinCwnd", UintegerValue(2));

    // Establish base delay measurement (Base Delay = 40 - 20 = 20ms)
    m_state->m_rcvTimestampValue = 40;
    m_state->m_rcvTimestampEchoReply = 20;
    cong->PktsAcked(m_state, m_segmentsAcked, m_rtt);

    // Set current measured delay based on test parameters
    m_state->m_rcvTimestampValue = m_rcvTimestampValue;
    m_state->m_rcvTimestampEchoReply = m_currentRcvTimestampEchoReply;
    cong->PktsAcked(m_state, m_segmentsAcked, m_rtt);

    uint32_t oldCwnd = m_state->m_cWnd.Get();

    // LEDBAT adjusts cwnd based on queuing delay relative to target:
    // - When queuing delay < target: cwnd increases (positive offset)
    // - When queuing delay > target: cwnd decreases (negative offset)
    // - cwnd is bounded by max (flightSizeBeforeAck + ALLOWED_INCREASE * MSS)
    //   and min (minCwnd * MSS)

    switch (m_testType)
    {
    case INCREMENT_TEST:
        // Timestamps: rcvTimestamp=80, echoReply=20
        // currentDelay = 80 - 20 = 60ms
        // baseDelay = 20ms (established earlier: 40 - 20)
        // queueDelay = currentDelay - baseDelay = 60 - 20 = 40ms
        // offset = target - queueDelay = 100 - 40 = 60ms
        // offset *= gain: offset = 60 * 1.0 = 60
        // oldCwnd = 8676 bytes
        // m_sndCwndCnt = offset * segmentsAcked * segmentSize
        //              = 60 * 2 * 1446 = 173520
        // inc = m_sndCwndCnt / (target * oldCwnd)
        //     = 173520 / (100 * 8676) = 0.2
        // newCwnd = oldCwnd + (inc * segmentSize)
        //         = 8676 + (0.2 * 1446) = 8676 + 289.2 = 8965.2 ≈ 8965
        // flightSizeBeforeAck = bytesInFlight + segmentsAcked * segmentSize
        //                     = 5784 + 2 * 1446 = 8676
        // maxCwnd = flightSizeBeforeAck + allowedIncrease * segmentSize
        //         = 8676 + 1.0 * 1446 = 10122
        // newCwnd = min(8965, 10122) = 8965
        // newCwnd = max(8965, 2892) = 8965
        // Final newCwnd = 8965 bytes
        // increment = inc * segmentSize = 0.2 * 1446 = 289
        cong->IncreaseWindow(m_state, m_segmentsAcked);
        NS_TEST_ASSERT_MSG_EQ(m_state->m_cWnd.Get(),
                              oldCwnd + 289,
                              "cWnd of 8676 should increase by 289");
        break;

    case MAX_CWND_TEST:
        // Timestamps: rcvTimestamp=80, echoReply=20
        // currentDelay = 80 - 20 = 60ms
        // baseDelay = 20ms (established earlier: 40 - 20)
        // queueDelay = currentDelay - baseDelay = 60 - 20 = 40ms
        // offset = target - queueDelay = 100 - 40 = 60ms
        // offset *= gain: offset = 60 * 1.0 = 60
        // oldCwnd = 8676 bytes, bytesInFlight = 4338 bytes
        // m_sndCwndCnt = offset * segmentsAcked * segmentSize
        //              = 60 * 2 * 1446 = 173520
        // inc = m_sndCwndCnt / (target * oldCwnd)
        //     = 173520 / (100 * 8676) = 0.2
        // newCwnd = oldCwnd + (inc * segmentSize)
        //         = 8676 + (0.2 * 1446) = 8676 + 289.2 = 8965.2 ≈ 8965
        // flightSizeBeforeAck = bytesInFlight + segmentsAcked * segmentSize
        //                     = 4338 + 2 * 1446 = 7230
        // maxCwnd = flightSizeBeforeAck + allowedIncrease * segmentSize
        //         = 7230 + 1.0 * 1446 = 8676
        // Final newCwnd = min(8965, 8676) = 8676 bytes (clamped by maxCwnd)
        cong->IncreaseWindow(m_state, m_segmentsAcked);
        NS_TEST_ASSERT_MSG_EQ(m_state->m_cWnd.Get(), 8676, "cWnd should be limited by maxCwnd");
        break;

    case DECREMENT_TEST:
        // Timestamps: rcvTimestamp=160, echoReply=20
        // currentDelay = 160 - 20 = 140ms
        // baseDelay = 20ms (established earlier: 40 - 20)
        // queueDelay = currentDelay - baseDelay = 140 - 20 = 120ms
        // offset = target - queueDelay = 100 - 120 = -20ms
        // offset *= gain: offset = -20 * 1.0 = -20
        // oldCwnd = 8676 bytes
        // m_sndCwndCnt = offset * segmentsAcked * segmentSize
        //              = -20 * 2 * 1446 = -57840
        // inc = m_sndCwndCnt / (target * oldCwnd)
        //     = -57840 / (100 * 8676) = -0.06667
        // newCwnd = oldCwnd + (inc * segmentSize)
        //         = 8676 + (-0.06667 * 1446) = 8676 - 96.4 = 8579.6 ≈ 8579
        // flightSizeBeforeAck = bytesInFlight + segmentsAcked * segmentSize
        //                     = 5784 + 2 * 1446 = 8676
        // maxCwnd = flightSizeBeforeAck + allowedIncrease * segmentSize
        //         = 8676 + 1.0 * 1446 = 10122
        // newCwnd = min(8579, 10122) = 8579
        // newCwnd = max(8579, 2892) = 8579
        // Final newCwnd = 8579 bytes
        // decrement = inc * segmentSize = -0.06667 * 1446 = -97
        cong->IncreaseWindow(m_state, m_segmentsAcked);
        NS_TEST_ASSERT_MSG_EQ(m_state->m_cWnd.Get(),
                              oldCwnd - 97,
                              "cWnd of 8676 should decrease by 97");
        break;

    case MIN_CWND_TEST:
        // Timestamps: rcvTimestamp=160, echoReply=20
        // currentDelay = 160 - 20 = 140ms
        // baseDelay = 20ms (established earlier: 40 - 20)
        // queueDelay = currentDelay - baseDelay = 140 - 20 = 120ms
        // offset = target - queueDelay = 100 - 120 = -20ms
        // offset *= gain: offset = -20 * 1.0 = -20
        // oldCwnd = 2892 bytes, minCwnd = 2 segments = 2892 bytes
        // m_sndCwndCnt = offset * segmentsAcked * segmentSize
        //              = -20 * 2 * 1446 = -57840
        // inc = m_sndCwndCnt / (target * oldCwnd)
        //     = -57840 / (100 * 2892) = -0.2
        // newCwnd = oldCwnd + (inc * segmentSize)
        //         = 2892 + (-0.2 * 1446) = 2892 - 289.2 = 2602.8 ≈ 2602
        // flightSizeBeforeAck = bytesInFlight + segmentsAcked * segmentSize
        //                     = 5784 + 2 * 1446 = 8676
        // maxCwnd = flightSizeBeforeAck + allowedIncrease * segmentSize
        //         = 8676 + 1.0 * 1446 = 10122
        // newCwnd = min(2602, 10122) = 2602
        // Final newCwnd = max(2602, 2892) = 2892 bytes (clamped by minCwnd)
        cong->IncreaseWindow(m_state, m_segmentsAcked);
        NS_TEST_ASSERT_MSG_EQ(m_state->m_cWnd.Get(),
                              2 * 1446,
                              "cWnd should be clamped to minCwnd (2892)");
        break;

    default:
        NS_FATAL_ERROR("Unknown Test Type");
        break;
    }

    Simulator::Destroy();
}

/**
 * @ingroup internet-test
 *
 * @brief TCP Ledbat TestSuite
 */
class TcpLedbatTestSuite : public TestSuite
{
  public:
    TcpLedbatTestSuite()
        : TestSuite("tcp-ledbat-test", Type::UNIT)
    {
        AddTestCase(new TcpLedbatToNewReno(2 * 1446,
                                           1446,
                                           4 * 1446,
                                           2,
                                           1446,
                                           MilliSeconds(100),
                                           "LEDBAT falls to New Reno for slowstart"),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpLedbatToNewReno(4 * 1446,
                                           1446,
                                           2 * 1446,
                                           2,
                                           1446,
                                           MilliSeconds(100),
                                           "LEDBAT falls to New Reno if timestamps are not found"),
                    TestCase::Duration::QUICK);
        AddTestCase(
            new TcpLedbatCongestionAvoidanceTest(6 * 1446,
                                                 1446,
                                                 4 * 1446,
                                                 2,
                                                 4 * 1446,
                                                 MilliSeconds(100),
                                                 80,
                                                 20,
                                                 TcpLedbatCongestionAvoidanceTest::INCREMENT_TEST,
                                                 "LEDBAT Congestion Avoidance Increment"),
            TestCase::Duration::QUICK);
        AddTestCase(new TcpLedbatCongestionAvoidanceTest(
                        6 * 1446,
                        1446,
                        4 * 1446,
                        2,
                        3 * 1446,
                        MilliSeconds(100),
                        80,
                        20,
                        TcpLedbatCongestionAvoidanceTest::MAX_CWND_TEST,
                        "LEDBAT Congestion Avoidance Increment (Limited by max_cwnd)"),
                    TestCase::Duration::QUICK);
        AddTestCase(
            new TcpLedbatCongestionAvoidanceTest(6 * 1446,
                                                 1446,
                                                 4 * 1446,
                                                 2,
                                                 4 * 1446,
                                                 MilliSeconds(100),
                                                 160,
                                                 20,
                                                 TcpLedbatCongestionAvoidanceTest::DECREMENT_TEST,
                                                 "LEDBAT Congestion Avoidance Decrement"),
            TestCase::Duration::QUICK);
        AddTestCase(new TcpLedbatCongestionAvoidanceTest(
                        2 * 1446,
                        1446,
                        4 * 1446,
                        2,
                        4 * 1446,
                        MilliSeconds(100),
                        160,
                        20,
                        TcpLedbatCongestionAvoidanceTest::MIN_CWND_TEST,
                        "LEDBAT Congestion Avoidance Decrement (Limited by min_cwnd)"),
                    TestCase::Duration::QUICK);
    }
};

static TcpLedbatTestSuite g_tcpledbatTest; //!< static var for test initialization
