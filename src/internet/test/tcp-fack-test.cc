/*
 * Copyright (c) 2018, 2025 NITK Surathkal
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Shikha Bakshi <shikhabakshi912@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *          Jayesh Akot <akotjayesh@gmail.com>
 */

/**
 * @file
 * @brief Unit test for the TCP Forward Acknowledgment (FACK) implementation.
 *
 * This unit test creates a short packet flow and forces four consecutive lost
 * segments, and verifies that the snd.fack variable is updated to the highest sequence
 * number present in the incoming SACK blocks, and that its external calculation of
 * awnd matches the internal state variable.
 */

#include "tcp-error-model.h"
#include "tcp-general-test.h"

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simple-channel.h"
#include "ns3/tcp-header.h"
#include "ns3/tcp-tx-buffer.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpFackTest");

/**
 * @ingroup internet-test
 * @brief Test case for Forward Acknowledgment(FACK).
 *
 * This test verifies the correctness of FACK-related state variables,
 * in particular the computation of sndFack and the FACK's inflight(awnd),
 * ensuring they match the expected behavior of the implementation.
 */
class TcpFackTest : public TcpGeneralTest
{
  public:
    /**
     * @brief Constructor.
     * @param congControl The TypeId of the congestion control algorithm used.
     * @param desc Description string for the test case.
     */
    TcpFackTest(TypeId congControl, const std::string& desc)
        : TcpGeneralTest(desc),
          m_congControlType(congControl)

    {
        m_congControlTypeId = congControl;
    }

  private:
    Ptr<TcpSeqErrorModel> m_errorModel; //!< Error model.
    uint32_t m_pktDropped{0};           //!< The Number of packets has been dropped.
    uint32_t m_startSeqToKill{4001};    //!< Sequence number of the first packet to drop.
    uint32_t m_seqToKill{0};            //!< Sequence number to drop.
    uint32_t m_pkts{4};                 //!< Number of packets to drop.
    uint32_t m_pktSize{1000};           //!< Sender Packet Size

    /**
     * @brief Count number of packets dropped
     * @param ipH IPv4 header of the dropped packet.
     * @param tcpH TCP header of the dropped packet.
     * @param p The dropped packet.
     */
    void PktDropped(const Ipv4Header& ipH, const TcpHeader& tcpH, Ptr<const Packet> p)
    {
        NS_LOG_FUNCTION(this << ipH << tcpH);
        m_pktDropped++;
    }

    void ConfigureEnvironment() override
    {
        TcpGeneralTest::ConfigureEnvironment();
        SetAppPktSize(1000);
        SetAppPktCount(10); // send 10 packets
    }

    void ConfigureProperties() override
    {
        TcpGeneralTest::ConfigureProperties();
        SetInitialCwnd(SENDER, 10); // start with 10 segments
        SetSegmentSize(SENDER, 1000);
        SetSegmentSize(RECEIVER, 1000);
    }

    Ptr<ErrorModel> CreateReceiverErrorModel() override
    {
        m_errorModel = CreateObject<TcpSeqErrorModel>();

        for (uint32_t i = 0; i < m_pkts; i++)
        {
            m_seqToKill = m_startSeqToKill + i * m_pktSize;
            m_errorModel->AddSeqToKill(SequenceNumber32(m_seqToKill));
            m_errorModel->SetDropCallback(MakeCallback(&TcpFackTest::PktDropped, this));
        }

        return m_errorModel;
    }

    Ptr<TcpSocketMsgBase> CreateSenderSocket(Ptr<Node> node) override
    {
        Ptr<TcpSocketMsgBase> sock = TcpGeneralTest::CreateSenderSocket(node);
        sock->SetAttribute("Fack", BooleanValue(true));
        return sock;
    }

    void RcvAck(const Ptr<const TcpSocketState> tcb, const TcpHeader& h, SocketWho who) override
    {
        if (tcb->m_cWnd.Get() >= tcb->m_ssThresh)
        {
            Ptr<TcpSocketBase> sock = DynamicCast<TcpSocketBase>(GetSenderSocket());
            uint32_t awnd = tcb->m_highTxMark.Get().GetValue() - sock->GetSndFack() +
                            sock->GetTxBuffer()->GetRetransmitsCount();

            NS_TEST_EXPECT_MSG_EQ(awnd, tcb->m_fackAwnd.Get(), "AWND is not correctly calculated");
        }
    }

    void ProcessedAck(const Ptr<const TcpSocketState> tcb,
                      const TcpHeader& h,
                      SocketWho who) override
    {
        if (!h.HasOption(TcpOption::SACK))
        {
            return;
        }

        Ptr<const TcpOption> opt = h.GetOption(TcpOption::SACK);
        auto sackOpt = DynamicCast<const TcpOptionSack>(opt);

        if (!sackOpt)
        {
            return;
        }

        TcpOptionSack::SackList sacks = sackOpt->GetSackList();
        SequenceNumber32 highest = h.GetAckNumber();

        for (const auto& s : sacks)
        {
            if (s.second > highest)
            {
                highest = s.second;
            }
        }
        auto sock = DynamicCast<TcpSocketBase>(GetSenderSocket());

        NS_TEST_EXPECT_MSG_EQ(sock->GetSndFack(),
                              highest.GetValue(),
                              "snd_fack did not advance to highest SACKed seq");
    }

  private:
    TypeId m_congControlType; //!< Congestion control algorithm type used for this test.
};

/**
 * @ingroup internet-test
 *
 * @brief Test suite for verifying the behavior of the TCP FACK implementation
 *        under controlled packet loss scenarios.
 *
 * Verifies:
 *  - snd.fack correctly tracks the highest SACKed sequence
 *  - awnd calculation matches FACK’s tracking
 */

class TcpFackTestSuite : public TestSuite
{
  public:
    TcpFackTestSuite()
        : TestSuite("tcp-fack-test", Type::UNIT)
    {
        TypeId cca = TcpNewReno::GetTypeId();

        AddTestCase(
            new TcpFackTest(cca, " Test snd.fack and awnd values after four packets are dropped."),
            Duration::QUICK);
    }
};

/**
 * @brief Static variable for test initialization
 */
static TcpFackTestSuite g_tcpFackTestSuite;
