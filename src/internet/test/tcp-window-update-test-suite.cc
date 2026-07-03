/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#include "tcp-general-test.h"

#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/tcp-header.h"
#include "ns3/uinteger.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpWindowUpdateTestSuite");

/**
 * @ingroup internet-test
 * @ingroup tests
 *
 * @brief Regression test for @issueid{125}.
 *
 * @issueid{125}: when the receiving application reads buffered data and
 * thereby frees up receive-buffer space, TCP must proactively send a
 * window-update ACK rather than leave a sender stalled on a zero (closed)
 * window until its persist timer (zero-window probe) fires. This matches
 * TcpSocketBase::SetRcvBufSize(), which, when the buffer is enlarged while
 * the connection is established, explicitly transmits an ACK to advertise
 * the reopened window.
 *
 * Scenario:
 *  - The receiver is created with a small RcvBufSize so that, after the sender
 *    fills it, the receiver advertises a window of 0 and the sender stalls.
 *  - The receiving application does NOT read while the buffer fills (the
 *    default ReceivePacket(), which drains the buffer, is overridden to do
 *    nothing).
 *  - At a fixed, deterministic time, well before the sender's persist timer
 *    could fire (the persist timeout is deliberately set very large), the
 *    receiving application reads the entire buffer via Recv(), freeing all
 *    receive-buffer space.
 *
 * Correct behaviour (asserted here): immediately after the application read,
 * and WITHOUT waiting for the sender's persist timer, the receiver transmits
 * an ACK advertising a non-zero window (a window update).
 */
class TcpWindowUpdateOnReadTest : public TcpGeneralTest
{
  public:
    /**
     * @brief Constructor.
     * @param desc Test description.
     */
    TcpWindowUpdateOnReadTest(const std::string& desc);

  protected:
    Ptr<TcpSocketMsgBase> CreateReceiverSocket(Ptr<Node> node) override;
    void ReceivePacket(Ptr<Socket> socket) override;
    void Tx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who) override;
    void ConfigureEnvironment() override;
    void ConfigureProperties() override;
    void FinalChecks() override;

  private:
    /**
     * @brief Scheduled application read that frees the entire receive buffer.
     */
    void AppRead();

    Time m_readTime;             //!< Time at which the application reads to free the window.
    bool m_windowClosed;         //!< True once the receiver has advertised a zero window.
    bool m_appHasRead;           //!< True once the scheduled application read has executed.
    bool m_windowUpdateSent;     //!< True if a window-update ACK was sent shortly after the read.
    Time m_windowUpdateDeadline; //!< Latest time a read-triggered update may legitimately appear.
};

TcpWindowUpdateOnReadTest::TcpWindowUpdateOnReadTest(const std::string& desc)
    : TcpGeneralTest(desc),
      m_readTime(Seconds(12)),
      m_windowClosed(false),
      m_appHasRead(false),
      m_windowUpdateSent(false),
      m_windowUpdateDeadline(Seconds(13))
{
}

void
TcpWindowUpdateOnReadTest::ConfigureEnvironment()
{
    TcpGeneralTest::ConfigureEnvironment();
    // Send more data than the receiver buffer can hold so that the window closes.
    SetAppPktCount(20);
    SetAppPktSize(500);
    SetMTU(500);
    SetTransmitStart(Seconds(2));
    SetPropagationDelay(MilliSeconds(50));
    // TcpGeneralTest runs until the event queue empties. With the fix in place,
    // the window update revives the sender, which then persist-probes the
    // (still-closed) window forever because the application reads only once;
    // bound the run well past the window-update deadline so it terminates.
    Simulator::Stop(Seconds(20));
}

void
TcpWindowUpdateOnReadTest::ConfigureProperties()
{
    TcpGeneralTest::ConfigureProperties();
    SetInitialCwnd(SENDER, 10);
    // A very large persist timeout guarantees that any window reopening observed
    // around m_readTime is caused by the application read, not by a zero-window
    // probe from the sender's persist timer.
    GetSenderSocket()->SetAttribute("PersistTimeout", TimeValue(Seconds(600)));
}

Ptr<TcpSocketMsgBase>
TcpWindowUpdateOnReadTest::CreateReceiverSocket(Ptr<Node> node)
{
    Ptr<TcpSocketMsgBase> socket = TcpGeneralTest::CreateReceiverSocket(node);

    // Disable window scaling so that TcpHeader::GetWindowSize() is the advertised
    // window in bytes and assertions can be made directly on it.
    socket->SetAttribute("WindowScaling", BooleanValue(false));
    // Small receive buffer: a single segment. After the sender fills it, the
    // receiver advertises a window of 0 and the sender stalls.
    socket->SetAttribute("RcvBufSize", UintegerValue(500));

    // The application reads (and frees the whole buffer) at a fixed, deterministic
    // time, long before the (600 s) persist timer could fire.
    Simulator::Schedule(m_readTime, &TcpWindowUpdateOnReadTest::AppRead, this);

    return socket;
}

void
TcpWindowUpdateOnReadTest::ReceivePacket(Ptr<Socket> socket)
{
    // Intentionally do NOT read here: we want the receive buffer to fill so the
    // window closes. The data is drained later, explicitly, in AppRead().
    NS_LOG_INFO("RECEIVER application notified of data but does not read (window must close)");
}

void
TcpWindowUpdateOnReadTest::AppRead()
{
    NS_LOG_INFO("RECEIVER application reads to free the receive buffer at "
                << Simulator::Now().As(Time::S));
    m_appHasRead = true;
    Ptr<Socket> socket = GetReceiverSocket();
    // Drain everything currently buffered, freeing receive-buffer space.
    while (Ptr<Packet> p = socket->Recv(0xffffffff, 0))
    {
        if (p->GetSize() == 0)
        {
            break;
        }
    }
}

void
TcpWindowUpdateOnReadTest::Tx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who)
{
    if (who != RECEIVER)
    {
        return;
    }

    NS_LOG_INFO("RECEIVER TX " << h << " size " << p->GetSize() << " at "
                               << Simulator::Now().As(Time::S));

    // Track that the receiver has advertised a closed (zero) window at least once
    // before the application read. This anchors the scenario: the window really
    // did close, so the sender really was stalled.
    if (!m_appHasRead && (h.GetWindowSize() == 0))
    {
        m_windowClosed = true;
    }

    // A window-update ACK triggered by the application read: an ACK carrying a
    // non-zero window, emitted shortly after the read and well before the
    // (600 s) persist timer. With a 500-byte buffer fully drained, the reopened
    // window is 500.
    if (m_appHasRead && Simulator::Now() <= m_windowUpdateDeadline && (h.GetWindowSize() > 0))
    {
        m_windowUpdateSent = true;
    }
}

void
TcpWindowUpdateOnReadTest::FinalChecks()
{
    NS_TEST_ASSERT_MSG_EQ(m_windowClosed,
                          true,
                          "Precondition failed: receiver never advertised a zero window, "
                          "so the sender was never stalled");
    NS_TEST_ASSERT_MSG_EQ(m_appHasRead,
                          true,
                          "Precondition failed: the scheduled application read did not run");
    // Reading data that frees receive-buffer space must elicit a window-update
    // ACK without waiting for the sender persist timer (@issueid{125}).
    NS_TEST_ASSERT_MSG_EQ(m_windowUpdateSent,
                          true,
                          "Receiver did not send a window-update ACK after the application "
                          "freed receive-buffer space (issue #125): the only relief is the "
                          "sender persist timer");
}

/**
 * @ingroup internet-test
 * @ingroup tests
 *
 * @brief TCP window-update-on-read TestSuite (@issueid{125}).
 */
class TcpWindowUpdateTestSuite : public TestSuite
{
  public:
    TcpWindowUpdateTestSuite()
        : TestSuite("tcp-window-update", Type::UNIT)
    {
        AddTestCase(new TcpWindowUpdateOnReadTest(
                        "window update ACK sent when application read frees rx buffer"),
                    TestCase::Duration::QUICK);
    }
};

static TcpWindowUpdateTestSuite g_tcpWindowUpdateTestSuite; //!< Static variable for test init
