/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/tcp-dctcp.h"
#include "ns3/tcp-socket-state.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpDctcpLossTestSuite");

/**
 * @ingroup internet-test
 * @ingroup tests
 *
 * @brief Regression test for @issueid{1012}.
 *
 * @issueid{1012}: "DcTcp behaves incorrectly with non-ECN routers".
 *
 * @RFC{8257} Section 3.5 ("Handling of Packet Loss") mandates that a DCTCP
 * sender MUST react to a loss episode (loss inferred, not signalled by ECN)
 * in the same way a conventional TCP would, i.e. it MUST reduce cwnd and
 * ssthresh as @RFC{5681} does (roughly halving them based on BytesInFlight).
 *
 * Without ECN-marking routers, @c m_alpha decays toward 0 (because
 * @c m_ackedBytesEcn stays 0), so the alpha-based reduction alone would leave
 * ssthresh at approximately @c m_cWnd on a packet loss.
 *
 * This test exercises GetSsThresh directly on a crafted TcpSocketState with
 * alpha == 0 (the non-ECN steady state) and asserts the @RFC{5681} behaviour.
 */
class TcpDctcpLossSsThreshTest : public TestCase
{
  public:
    /**
     * @brief Constructor.
     *
     * @param cWnd Congestion window (bytes) at the time of the loss event.
     * @param segmentSize Segment size in bytes.
     * @param bytesInFlight Bytes in flight reported to GetSsThresh.
     * @param name Name of the test.
     */
    TcpDctcpLossSsThreshTest(uint32_t cWnd,
                             uint32_t segmentSize,
                             uint32_t bytesInFlight,
                             const std::string& name);

  private:
    void DoRun() override;

    uint32_t m_cWnd;          //!< Congestion window at loss time
    uint32_t m_segmentSize;   //!< Segment size
    uint32_t m_bytesInFlight; //!< Bytes in flight passed to GetSsThresh
};

TcpDctcpLossSsThreshTest::TcpDctcpLossSsThreshTest(uint32_t cWnd,
                                                   uint32_t segmentSize,
                                                   uint32_t bytesInFlight,
                                                   const std::string& name)
    : TestCase(name),
      m_cWnd(cWnd),
      m_segmentSize(segmentSize),
      m_bytesInFlight(bytesInFlight)
{
}

void
TcpDctcpLossSsThreshTest::DoRun()
{
    // Craft a sender state representing the moment a packet loss is
    // detected. With non-ECN routers the DCTCP congestion estimate alpha has
    // decayed to (approximately) zero.
    Ptr<TcpSocketState> state = CreateObject<TcpSocketState>();
    state->m_cWnd = m_cWnd;
    state->m_segmentSize = m_segmentSize;
    state->m_ssThresh = m_cWnd;
    // Mark this as a packet loss event (as opposed to an ECN-CE reduction), so a
    // correct DCTCP implementation applies @RFC{5681} loss-based backoff here.
    state->m_congState = TcpSocketState::CA_LOSS;

    Ptr<TcpDctcp> cong = CreateObject<TcpDctcp>();
    // Force alpha to its non-ECN steady state of 0. DctcpAlphaOnInit must be
    // set before Init()/use; the default initial alpha is 1.0.
    cong->SetAttribute("DctcpAlphaOnInit", DoubleValue(0.0));

    // GetSsThresh() is invoked on entering CWR following a congestion event.
    // For a packet loss, @RFC{8257} Section 3.5 requires the same
    // reduction as @RFC{5681}: ssthresh = max(BytesInFlight / 2, 2 * SMSS).
    uint32_t newSsThresh = cong->GetSsThresh(state, m_bytesInFlight);

    // @RFC{5681} loss-based expectation.
    uint32_t expectedSsThresh = std::max(m_bytesInFlight / 2, 2 * m_segmentSize);

    // Allow a tolerance of one segment to accommodate rounding.
    uint32_t tolerance = m_segmentSize;

    NS_LOG_INFO("cWnd=" << m_cWnd << " bytesInFlight=" << m_bytesInFlight << " GetSsThresh="
                        << newSsThresh << " expected(~half)=" << expectedSsThresh);

    // With alpha == 0, @RFC{5681} loss-based backoff must yield
    // ~bytesInFlight/2 (@issueid{1012}).
    NS_TEST_ASSERT_MSG_EQ_TOL(
        newSsThresh,
        expectedSsThresh,
        tolerance,
        "DCTCP did not apply RFC-5681 loss-based backoff on a packet "
        "loss: ssthresh should be ~BytesInFlight/2 but is ~cwnd (issue #1012)");
}

/**
 * @ingroup internet-test
 * @ingroup tests
 *
 * @brief TCP DCTCP non-ECN loss back-off TestSuite (regression for @issueid{1012}).
 *
 * Checks that DCTCP performs @RFC{5681} loss-based congestion-window reduction
 * on a packet loss. See TcpDctcpLossSsThreshTest.
 */
class TcpDctcpLossTestSuite : public TestSuite
{
  public:
    TcpDctcpLossTestSuite()
        : TestSuite("tcp-dctcp-loss", Type::UNIT)
    {
        // cWnd = 20 segments, bytesInFlight = 20 segments. Loss-based recovery
        // should drop ssthresh to ~10 segments; the bug leaves it at ~20.
        AddTestCase(new TcpDctcpLossSsThreshTest(20 * 1446,
                                                 1446,
                                                 20 * 1446,
                                                 "DCTCP backs off to ~BytesInFlight/2 on "
                                                 "packet loss with alpha=0"),
                    TestCase::Duration::QUICK);
    }
};

static TcpDctcpLossTestSuite g_tcpDctcpLossTestSuite; //!< Static instance for test registration
