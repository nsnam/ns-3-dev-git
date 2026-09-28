/*
 * Copyright (c) 2015 Natale Patriciello <natale.patriciello@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#include "tcp-error-model.h"
#include "tcp-general-test.h"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simulator.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpZeroWindowTestSuite");

/**
 * @ingroup internet-test
 *
 * @brief Testing the congestion avoidance increment on TCP ZeroWindow
 */
class TcpZeroWindowTest : public TcpGeneralTest
{
  public:
    /**
     * @brief Constructor.
     * @param desc Test description.
     */
    TcpZeroWindowTest(const std::string& desc);

  protected:
    // virtual void ReceivePacket (Ptr<Socket> socket);
    Ptr<TcpSocketMsgBase> CreateReceiverSocket(Ptr<Node> node) override;

    void Tx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who) override;
    void Rx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who) override;
    void ProcessedAck(const Ptr<const TcpSocketState> tcb,
                      const TcpHeader& h,
                      SocketWho who) override;
    void NormalClose(SocketWho who) override;
    void FinalChecks() override;

    void ConfigureEnvironment() override;
    void ConfigureProperties() override;

    /**
     * @brief Increase the receiver buffer size.
     */
    void IncreaseBufSize();

  protected:
    EventId m_receivePktEvent; //!< Receive packet event.
    bool m_zeroWindowProbe;    //!< ZeroWindow probe.
    bool m_windowUpdated;      //!< Window updated.
    bool m_senderFinished;     //!< Send finished.
    bool m_receiverFinished;   //!< Receiver finished.
};

TcpZeroWindowTest::TcpZeroWindowTest(const std::string& desc)
    : TcpGeneralTest(desc),
      m_zeroWindowProbe(false),
      m_windowUpdated(false),
      m_senderFinished(false),
      m_receiverFinished(false)
{
}

void
TcpZeroWindowTest::ConfigureEnvironment()
{
    TcpGeneralTest::ConfigureEnvironment();
    SetAppPktCount(20);
    SetMTU(500);
    SetTransmitStart(Seconds(2));
    SetPropagationDelay(MilliSeconds(50));
}

void
TcpZeroWindowTest::ConfigureProperties()
{
    TcpGeneralTest::ConfigureProperties();
    SetInitialCwnd(SENDER, 10);
}

void
TcpZeroWindowTest::IncreaseBufSize()
{
    SetRcvBufSize(RECEIVER, 2500);
}

Ptr<TcpSocketMsgBase>
TcpZeroWindowTest::CreateReceiverSocket(Ptr<Node> node)
{
    Ptr<TcpSocketMsgBase> socket = TcpGeneralTest::CreateReceiverSocket(node);

    socket->SetAttribute("RcvBufSize", UintegerValue(0));
    Simulator::Schedule(Seconds(10), &TcpZeroWindowTest::IncreaseBufSize, this);

    return socket;
}

void
TcpZeroWindowTest::Tx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who)
{
    if (who == SENDER)
    {
        NS_LOG_INFO("\tSENDER TX " << h << " size " << p->GetSize());

        if (Simulator::Now().GetSeconds() <= 6.0)
        {
            NS_TEST_ASSERT_MSG_EQ(p->GetSize(), 0, "Data packet sent anyway");
        }
        else if (Simulator::Now().GetSeconds() > 6.0 && Simulator::Now().GetSeconds() <= 7.0)
        {
            NS_TEST_ASSERT_MSG_EQ(m_zeroWindowProbe, false, "Sent another probe");

            if (!m_zeroWindowProbe)
            {
                NS_TEST_ASSERT_MSG_EQ(p->GetSize(), 1, "Data packet sent instead of window probe");
                NS_TEST_ASSERT_MSG_EQ(h.GetSequenceNumber(),
                                      SequenceNumber32(1),
                                      "Data packet sent instead of window probe");
                m_zeroWindowProbe = true;
            }
        }
        else if (Simulator::Now().GetSeconds() > 7.0 && Simulator::Now().GetSeconds() < 10.0)
        {
            NS_FATAL_ERROR("No packets should be sent before the window update");
        }
    }
    else if (who == RECEIVER)
    {
        NS_LOG_INFO("\tRECEIVER TX " << h << " size " << p->GetSize());

        if (h.GetFlags() & TcpHeader::SYN)
        {
            NS_TEST_ASSERT_MSG_EQ(h.GetWindowSize(),
                                  0,
                                  "RECEIVER window size is not 0 in the SYN-ACK");
        }

        if (Simulator::Now().GetSeconds() > 6.0 && Simulator::Now().GetSeconds() <= 7.0)
        {
            NS_TEST_ASSERT_MSG_EQ(h.GetSequenceNumber(),
                                  SequenceNumber32(1),
                                  "Data packet sent instead of window probe");
            NS_TEST_ASSERT_MSG_EQ(h.GetWindowSize(), 0, "No zero window advertised by RECEIVER");
        }
        else if (Simulator::Now().GetSeconds() > 7.0 && Simulator::Now().GetSeconds() < 10.0)
        {
            NS_FATAL_ERROR("No packets should be sent before the window update");
        }
        else if (Simulator::Now().GetSeconds() >= 10.0)
        {
            NS_TEST_ASSERT_MSG_EQ(h.GetWindowSize(), 2500, "Receiver window not updated");
        }
    }

    NS_TEST_ASSERT_MSG_EQ(GetTcb(SENDER)->m_congState.Get(),
                          TcpSocketState::CA_OPEN,
                          "Sender State is not OPEN");
    NS_TEST_ASSERT_MSG_EQ(GetTcb(RECEIVER)->m_congState.Get(),
                          TcpSocketState::CA_OPEN,
                          "Receiver State is not OPEN");
}

void
TcpZeroWindowTest::Rx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who)
{
    if (who == SENDER)
    {
        NS_LOG_INFO("\tSENDER RX " << h << " size " << p->GetSize());

        if (h.GetFlags() & TcpHeader::SYN)
        {
            NS_TEST_ASSERT_MSG_EQ(h.GetWindowSize(),
                                  0,
                                  "RECEIVER window size is not 0 in the SYN-ACK");
        }

        if (Simulator::Now().GetSeconds() >= 10.0)
        {
            NS_TEST_ASSERT_MSG_EQ(h.GetWindowSize(), 2500, "Receiver window not updated");
            m_windowUpdated = true;
        }
    }
    else if (who == RECEIVER)
    {
        NS_LOG_INFO("\tRECEIVER RX " << h << " size " << p->GetSize());
    }
}

void
TcpZeroWindowTest::NormalClose(SocketWho who)
{
    if (who == SENDER)
    {
        m_senderFinished = true;
    }
    else if (who == RECEIVER)
    {
        m_receiverFinished = true;
    }
}

void
TcpZeroWindowTest::FinalChecks()
{
    NS_TEST_ASSERT_MSG_EQ(m_zeroWindowProbe, true, "Zero window probe not sent");
    NS_TEST_ASSERT_MSG_EQ(m_windowUpdated, true, "Window has not updated during the connection");
    NS_TEST_ASSERT_MSG_EQ(m_senderFinished, true, "Connection not closed successfully (SENDER)");
    NS_TEST_ASSERT_MSG_EQ(m_receiverFinished,
                          true,
                          "Connection not closed successfully (RECEIVER)");
}

void
TcpZeroWindowTest::ProcessedAck(const Ptr<const TcpSocketState> tcb,
                                const TcpHeader& h,
                                SocketWho who)
{
    if (who == SENDER)
    {
        if (h.GetFlags() & TcpHeader::SYN)
        {
            EventId persistentEvent = GetPersistentEvent(SENDER);
            NS_TEST_ASSERT_MSG_EQ(persistentEvent.IsPending(),
                                  true,
                                  "Persistent event not started");
        }
    }
    else if (who == RECEIVER)
    {
    }
}

/**
 * @ingroup internet-test
 *
 * Regression test for SIGSEGV in TcpSocketBase::PersistTimeout when the
 * receiver's window is zero and the sender has no unsent data (issue 1326).
 * Verifies that the sender sends zero-length window probes with an already
 * acknowledged sequence number and that the receiver acknowledges them.
 */
class TcpPersistEmptyTxBufferTest : public TcpGeneralTest
{
  public:
    /**
     * @brief Constructor.
     * @param desc Test description.
     */
    TcpPersistEmptyTxBufferTest(const std::string& desc);

  protected:
    void ConfigureEnvironment() override;
    void ConfigureProperties() override;
    Ptr<TcpSocketMsgBase> CreateReceiverSocket(Ptr<Node> node) override;
    void ReceivePacket(Ptr<Socket> socket) override;
    void Tx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who) override;
    void Rx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who) override;
    void FinalChecks() override;

  private:
    uint32_t m_windowProbeCount{0}; //!< Zero-window probes sent by the sender.
    uint32_t m_probeAckCount{0};    //!< ACKs received by the sender while probing.
};

TcpPersistEmptyTxBufferTest::TcpPersistEmptyTxBufferTest(const std::string& desc)
    : TcpGeneralTest(desc)
{
}

void
TcpPersistEmptyTxBufferTest::ConfigureEnvironment()
{
    TcpGeneralTest::ConfigureEnvironment();
    SetAppPktSize(100);
    SetAppPktCount(1);
    // Keep the sender application from closing the connection before the stop time
    SetAppPktInterval(Seconds(1000));
    SetMTU(500);
    SetTransmitStart(Seconds(2));
    SetPropagationDelay(MilliSeconds(10));
    Simulator::Stop(Seconds(30));
}

void
TcpPersistEmptyTxBufferTest::ConfigureProperties()
{
    TcpGeneralTest::ConfigureProperties();
    SetInitialCwnd(SENDER, 10);
    // Several persist timer firings (with exponential backoff) fit before the stop time
    GetSenderSocket()->SetAttribute("PersistTimeout", TimeValue(Seconds(1)));
}

Ptr<TcpSocketMsgBase>
TcpPersistEmptyTxBufferTest::CreateReceiverSocket(Ptr<Node> node)
{
    Ptr<TcpSocketMsgBase> socket = TcpGeneralTest::CreateReceiverSocket(node);
    // The single 100-byte segment fills the receive buffer and closes the window
    socket->SetAttribute("RcvBufSize", UintegerValue(100));
    return socket;
}

void
TcpPersistEmptyTxBufferTest::ReceivePacket(Ptr<Socket> socket [[maybe_unused]])
{
    // Do not read, so that the receive window stays closed
}

void
TcpPersistEmptyTxBufferTest::Tx(const Ptr<const Packet> p, const TcpHeader& h, SocketWho who)
{
    // The data segment is acknowledged before the first persist timeout
    // expires, so every probe is sent with an empty tx buffer.
    if (who == SENDER && Simulator::Now() > Seconds(3))
    {
        NS_TEST_ASSERT_MSG_EQ(p->GetSize(), 0, "Window probe should carry no data");
        NS_TEST_ASSERT_MSG_EQ(h.GetSequenceNumber(),
                              SequenceNumber32(100),
                              "Window probe should use an already acknowledged sequence number");
        ++m_windowProbeCount;
    }
}

void
TcpPersistEmptyTxBufferTest::Rx(const Ptr<const Packet> p [[maybe_unused]],
                                const TcpHeader& h,
                                SocketWho who)
{
    if (who == SENDER && Simulator::Now() > Seconds(3))
    {
        NS_TEST_ASSERT_MSG_EQ(h.GetWindowSize(), 0, "Receiver window should stay closed");
        ++m_probeAckCount;
    }
}

void
TcpPersistEmptyTxBufferTest::FinalChecks()
{
    NS_TEST_ASSERT_MSG_GT_OR_EQ(m_windowProbeCount, 2, "Expected multiple zero-window probes");
    NS_TEST_ASSERT_MSG_EQ(m_probeAckCount,
                          m_windowProbeCount,
                          "Each zero-window probe should be acknowledged");
}

/**
 * @ingroup internet-test
 *
 * @brief TCP ZeroWindow TestSuite
 */
class TcpZeroWindowTestSuite : public TestSuite
{
  public:
    TcpZeroWindowTestSuite()
        : TestSuite("tcp-zero-window-test", Type::UNIT)
    {
        AddTestCase(new TcpZeroWindowTest("zero window test"), TestCase::Duration::QUICK);
        AddTestCase(new TcpPersistEmptyTxBufferTest(
                        "zero window probe does not crash on drained tx buffer"),
                    TestCase::Duration::QUICK);
    }
};

static TcpZeroWindowTestSuite g_tcpZeroWindowTestSuite; //!< Static variable for test initialization
