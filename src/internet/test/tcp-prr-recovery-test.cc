/*
 * Copyright (c) 2018 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Viyom Mittal <viyommittal@gmail.com>
 *         Vivek Jain <jain.vivek.anand@gmail.com>
 *         Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *
 */

#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-prr-recovery.h"
#include "ns3/tcp-recovery-ops.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpPrrRecoveryTestSuite");

/**
 * @brief PRR Recovery algorithm test
 */
class PrrRecoveryTest : public TestCase
{
  public:
    /**
     * @brief Constructor.
     * @param cWnd Congestion window.
     * @param segmentSize Segment size.
     * @param ssThresh Slow Start Threshold.
     * @param unAckDataCount Unacknowledged data at the start of recovery.
     * @param bytesInFlight Current bytes in flight.
     * @param m_deliveredBytes Bytes SACKed on last acknowledgment.
     * @param bytesSent Bytes sent while in recovery phase.
     * @param name Test description.
     */
    PrrRecoveryTest(uint32_t cWnd,
                    uint32_t segmentSize,
                    uint32_t ssThresh,
                    uint32_t unAckDataCount,
                    uint32_t bytesInFlight,
                    uint32_t m_deliveredBytes,
                    uint32_t bytesSent,
                    const std::string& name);

  private:
    void DoRun() override;

    uint32_t m_cWnd;           //!< Congestion window.
    uint32_t m_segmentSize;    //!< Segment size.
    uint32_t m_ssThresh;       //!< Slow Start Threshold.
    uint32_t m_unAckDataCount; //!< Unacknowledged data at the start of recovery.
    uint32_t m_bytesInFlight;  //!< Current bytes in flight.
    uint32_t m_deliveredBytes; //!< Bytes SACKed on last acknowledgment.
    uint32_t m_bytesSent;      //!< Bytes sent while in recovery phase.

    Ptr<TcpSocketState> m_state; //!< TCP socket state.
};

PrrRecoveryTest::PrrRecoveryTest(uint32_t cWnd,
                                 uint32_t segmentSize,
                                 uint32_t ssThresh,
                                 uint32_t unAckDataCount,
                                 uint32_t bytesInFlight,
                                 uint32_t deliveredBytes,
                                 uint32_t bytesSent,
                                 const std::string& name)
    : TestCase(name),
      m_cWnd(cWnd),
      m_segmentSize(segmentSize),
      m_ssThresh(ssThresh),
      m_unAckDataCount(unAckDataCount),
      m_bytesInFlight(bytesInFlight),
      m_deliveredBytes(deliveredBytes),
      m_bytesSent(bytesSent)
{
}

void
PrrRecoveryTest::DoRun()
{
    m_state = CreateObject<TcpSocketState>();

    m_state->m_cWnd = m_cWnd;
    m_state->m_cWndInfl = m_cWnd;
    m_state->m_segmentSize = m_segmentSize;
    m_state->m_ssThresh = m_ssThresh;
    m_state->m_bytesInFlight = m_bytesInFlight;
    // This test drives DoRecovery with deliveredBytes = 0 and relies on PRR's
    // "+1 SMSS per duplicate ACK" DeliveredData estimate, which (per RFC 9937
    // Section 6.2) applies only to connections without SACK.
    m_state->m_sackEnabled = false;

    Ptr<TcpPrrRecovery> recovery = CreateObject<TcpPrrRecovery>();

    recovery->EnterRecovery(m_state, 3, m_unAckDataCount, 0, 0);

    NS_TEST_ASSERT_MSG_GT_OR_EQ(m_state->m_cWnd.Get(),
                                m_cWnd + m_segmentSize,
                                "There should be at least one transmission on entering recovery");

    for (uint32_t iterator = 0; iterator < m_bytesSent;)
    {
        recovery->UpdateBytesSent(m_segmentSize);
        iterator += m_segmentSize;
    }

    m_bytesInFlight += m_state->m_cWnd.Get() - m_cWnd;
    m_state->m_bytesInFlight = m_bytesInFlight;
    m_cWnd = m_state->m_cWnd.Get();
    recovery->DoRecovery(m_state, m_deliveredBytes, false);

    if (m_bytesInFlight > m_state->m_ssThresh)
    {
        NS_TEST_ASSERT_MSG_LT_OR_EQ(
            m_state->m_cWnd.Get(),
            m_cWnd,
            "Updated cwnd should be less than or equal to the existing cwnd");
    }
    else
    {
        NS_TEST_ASSERT_MSG_GT_OR_EQ(
            m_state->m_cWnd.Get(),
            m_cWnd,
            "Updated cwnd should be greater than or equal to the existing cwnd");
    }
}

/**
 * @ingroup internet-test
 *
 * @brief PRR Recovery regression test for integer overflow and unsigned wrap
 *
 * Regression tests for issue #1282.  Calls TcpPrrRecovery::DoRecovery directly and
 * checks the resulting cwnd * against a value computed with correct 64-bit / signed
 * arithmetic. The two scenarios exercise the arithmetic defects fixed in
 * tcp-prr-recovery.cc (MR !2807):
 *  - the proportional branch (inflight > ssThresh), where the product
 *    m_prrDelivered * m_ssThresh overflowed a 32-bit integer; and
 *  - the PRR-CRB branch (inflight <= ssThresh), where m_prrDelivered - m_prrOut
 *    wrapped when m_prrOut exceeded m_prrDelivered.
 */
class PrrRecoveryArithmeticTest : public TestCase
{
  public:
    /**
     * @brief Constructor.
     * @param segmentSize Segment size.
     * @param ssThresh Slow Start Threshold.
     * @param bytesInFlight Bytes in flight at recovery entry (also the RecoverFS).
     * @param bytesSentInRecovery Bytes reported sent (via UpdateBytesSent) before the tested ACK.
     * @param deliveredBytes Bytes delivered on the tested (non-dupack) acknowledgment.
     * @param expectedCWnd Expected cwnd after the tested acknowledgment.
     * @param name Test description.
     */
    PrrRecoveryArithmeticTest(uint32_t segmentSize,
                              uint32_t ssThresh,
                              uint32_t bytesInFlight,
                              uint32_t bytesSentInRecovery,
                              uint32_t deliveredBytes,
                              uint32_t expectedCWnd,
                              const std::string& name);

  private:
    void DoRun() override;

    uint32_t m_segmentSize;         //!< Segment size.
    uint32_t m_ssThresh;            //!< Slow Start Threshold.
    uint32_t m_bytesInFlight;       //!< Bytes in flight at recovery entry.
    uint32_t m_bytesSentInRecovery; //!< Bytes reported sent before the tested ACK.
    uint32_t m_deliveredBytes;      //!< Bytes delivered on the tested ACK.
    uint32_t m_expectedCWnd;        //!< Expected cwnd after the tested ACK.
};

PrrRecoveryArithmeticTest::PrrRecoveryArithmeticTest(uint32_t segmentSize,
                                                     uint32_t ssThresh,
                                                     uint32_t bytesInFlight,
                                                     uint32_t bytesSentInRecovery,
                                                     uint32_t deliveredBytes,
                                                     uint32_t expectedCWnd,
                                                     const std::string& name)
    : TestCase(name),
      m_segmentSize(segmentSize),
      m_ssThresh(ssThresh),
      m_bytesInFlight(bytesInFlight),
      m_bytesSentInRecovery(bytesSentInRecovery),
      m_deliveredBytes(deliveredBytes),
      m_expectedCWnd(expectedCWnd)
{
}

void
PrrRecoveryArithmeticTest::DoRun()
{
    Ptr<TcpSocketState> state = CreateObject<TcpSocketState>();
    state->m_segmentSize = m_segmentSize;
    state->m_ssThresh = m_ssThresh;
    state->m_bytesInFlight = m_bytesInFlight;
    // These regression scenarios rely on the "+1 SMSS per duplicate ACK"
    // DeliveredData estimate (the "dupack bump"), which PRR applies only to
    // connections without SACK (RFC 9937 Section 6.2).
    state->m_sackEnabled = false;

    Ptr<TcpPrrRecovery> recovery = CreateObject<TcpPrrRecovery>();

    // EnterRecovery sets RecoverFS = bytesInFlight and performs the first DoRecovery.
    recovery->EnterRecovery(state, 3, m_bytesInFlight, 0, 0);

    // Report data sent during recovery, advancing m_prrOut one segment at a time.
    for (uint32_t sent = 0; sent < m_bytesSentInRecovery; sent += m_segmentSize)
    {
        recovery->UpdateBytesSent(m_segmentSize);
    }

    // The acknowledgment under test (not a dupack, so deliveredBytes is used as-is).
    recovery->DoRecovery(state, m_deliveredBytes, false);

    NS_TEST_ASSERT_MSG_EQ(state->m_cWnd.Get(),
                          m_expectedCWnd,
                          "cwnd does not match value computed with correct arithmetic");
}

/**
 * @ingroup internet-test
 *
 * @brief PRR Recovery TestSuite
 */
class PrrRecoveryTestSuite : public TestSuite
{
  public:
    PrrRecoveryTestSuite()
        : TestSuite("tcp-prr-recovery-test", Type::UNIT)
    {
        AddTestCase(new PrrRecoveryTest(
                        3000,
                        500,
                        2500,
                        3000,
                        3000,
                        500,
                        1000,
                        "Prr test on cWnd when bytesInFlight is greater than ssThresh with SSRB"),
                    TestCase::Duration::QUICK);
        AddTestCase(new PrrRecoveryTest(
                        1000,
                        500,
                        2500,
                        3000,
                        1000,
                        500,
                        1000,
                        "Prr test on cWnd when bytesInFlight is lower than ssThresh with SSRB"),
                    TestCase::Duration::QUICK);
        AddTestCase(new PrrRecoveryTest(
                        3000,
                        500,
                        2500,
                        3000,
                        3000,
                        500,
                        1000,
                        "Prr test on cWnd when bytesInFlight is greater than ssThresh with CRB"),
                    TestCase::Duration::QUICK);
        AddTestCase(new PrrRecoveryTest(
                        1000,
                        500,
                        2500,
                        3000,
                        1000,
                        500,
                        1000,
                        "Prr test on cWnd when bytesInFlight is lower than ssThresh with CRB"),
                    TestCase::Duration::QUICK);

        // Regression: proportional branch (inflight > ssThresh) must not overflow a
        // 32-bit intermediate value. EnterRecovery sets RecoverFS = 200000 and (via the
        // dupack bump) m_prrDelivered = segmentSize = 1000; the tested ACK delivers
        // 50000 bytes, so m_prrDelivered becomes 51000. Correct arithmetic gives
        // ceil(51000 * 100000 / 200000) = 25500 and cwnd = 200000 + 25500 = 225500.
        // Before the fix, 51000 * 100000 = 5.1e9 overflowed the uint32_t product.
        AddTestCase(new PrrRecoveryArithmeticTest(
                        1000,
                        100000,
                        200000,
                        0,
                        50000,
                        225500,
                        "Prr proportional branch must not overflow 32-bit product"),
                    TestCase::Duration::QUICK);

        // Regression: PRR-CRB branch (inflight <= ssThresh) must not wrap the unsigned
        // subtraction m_prrDelivered - m_prrOut. After EnterRecovery m_prrDelivered is
        // 1000; 50000 bytes are reported sent so m_prrOut is 50000; the tested ACK
        // delivers 1000 bytes (m_prrDelivered -> 2000). Signed arithmetic gives
        // max(2000 - 50000, 1000) = 1000 and cwnd = 50000 + 1000 = 51000. Before the
        // fix, the unsigned subtraction wrapped.
        AddTestCase(
            new PrrRecoveryArithmeticTest(1000,
                                          100000,
                                          50000,
                                          50000,
                                          1000,
                                          51000,
                                          "Prr CRB branch must not wrap unsigned subtraction"),
            TestCase::Duration::QUICK);
    }
};

static PrrRecoveryTestSuite g_TcpPrrRecoveryTest; //!< Static variable for test initialization
