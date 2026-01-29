/*
 * Copyright (c) 2011 The Boeing Company
 * Copyright (c) 2026 Tokushima University, Japan: Collision test and refactoring
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Gary Pei <guangyu.pei@boeing.com>
 *          Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */
#include "ns3/log.h"
#include "ns3/lr-wpan-mac.h"
#include "ns3/lr-wpan-phy.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/single-model-spectrum-channel.h"
#include "ns3/test.h"

using namespace ns3;
using namespace ns3::lrwpan;

/**
 * @ingroup lr-wpan-test
 * @ingroup tests
 *
 * @brief LrWpan PLME and PD Interfaces Test
 */
class LrWpanPlmeAndPdInterfaceTestCase : public TestCase
{
  public:
    LrWpanPlmeAndPdInterfaceTestCase();
    ~LrWpanPlmeAndPdInterfaceTestCase() override;

  private:
    uint16_t m_receivedPackets{0}; //!< Counts the number of received packets
    void DoRun() override;

    /**
     * @brief Receives a PdData indication
     * @param psduLength The PSDU length.
     * @param p The packet.
     * @param lqi The LQI.
     * @param rssi The RSSI
     */
    void ReceivePdDataIndication(uint32_t psduLength, Ptr<Packet> p, uint8_t lqi, int8_t rssi);
};

/**
 * @ingroup lr-wpan-test
 * @ingroup tests
 *
 * @brief Test the collision of two packets and the reaction ( packet drop )
 *        of this collision in the packet preamble
 */
class LrWpanPhyCollisionTestCase : public TestCase
{
  public:
    LrWpanPhyCollisionTestCase();
    ~LrWpanPhyCollisionTestCase() override;

  private:
    uint16_t m_receivedPackets{0}; //!< Counts the number of received packets
    int8_t m_receivedRssi{0};      //!< Saves the value of the received packet RSSI
    uint16_t m_packetDroppedByCollision{
        0}; //!< Counts the number of packets dropped due to collision
    void DoRun() override;

    /**
     * @brief Receives a PdData indication
     * @param psduLength The PSDU length.
     * @param p The packet.
     * @param lqi The LQI.
     * @param rssi The RSSI
     */
    void ReceivePdDataIndication(uint32_t psduLength, Ptr<Packet> p, uint8_t lqi, int8_t rssi);

    /**
     * @brief Callback for packet drop due to collision
     * @param p The packet.
     */
    void PacketCollisionDrop(Ptr<const Packet> p);
};

LrWpanPlmeAndPdInterfaceTestCase::LrWpanPlmeAndPdInterfaceTestCase()
    : TestCase("PLME and PD SAP IEEE 802.15.4 interfaces test")
{
}

LrWpanPlmeAndPdInterfaceTestCase::~LrWpanPlmeAndPdInterfaceTestCase()
{
}

void
LrWpanPlmeAndPdInterfaceTestCase::ReceivePdDataIndication(uint32_t psduLength,
                                                          Ptr<Packet> p,
                                                          uint8_t lqi,
                                                          int8_t rssi)
{
    NS_LOG_UNCOND("At: " << Simulator::Now() << " Received frame size: " << psduLength
                         << " LQI: " << static_cast<uint16_t>(lqi)
                         << " RSSI: " << static_cast<int16_t>(rssi) << " dBm.");

    m_receivedPackets++;
}

void
LrWpanPlmeAndPdInterfaceTestCase::DoRun()
{
    // This test create two Lr-WPAN PHYs and tests a simple transmission and reception
    // of a packet. It tests that the PHY interfaces (PLME primitives) are working.

    LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC));
    LogComponentEnable("LrWpanPhy", LOG_LEVEL_ALL);

    Ptr<LrWpanPhy> sender = CreateObject<LrWpanPhy>();
    Ptr<LrWpanPhy> receiver = CreateObject<LrWpanPhy>();

    // Add the SpectrumPhy to a channel and set the initial state of the PHYs
    // This is typically taken care by the NetDevice but in this case we are using the PHY directly.
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    sender->SetChannel(channel);
    receiver->SetChannel(channel);
    channel->AddRx(receiver);
    sender->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_TX_ON);
    receiver->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);

    receiver->SetPdDataIndicationCallback(
        MakeCallback(&LrWpanPlmeAndPdInterfaceTestCase::ReceivePdDataIndication, this));

    Ptr<Packet> p = Create<Packet>(10); // Packet with 10 bytes
    Simulator::Schedule(Seconds(1), &LrWpanPhy::PdDataRequest, sender, p->GetSize(), p);

    Simulator::Stop(Seconds(5));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_GT(m_receivedPackets,
                          0,
                          "Error,the number of expected received packets should be more than 0");
    Simulator::Destroy();
}

LrWpanPhyCollisionTestCase::LrWpanPhyCollisionTestCase()
    : TestCase("Collision of two IEEE 802.15.4 frames test")
{
}

LrWpanPhyCollisionTestCase::~LrWpanPhyCollisionTestCase()
{
}

void
LrWpanPhyCollisionTestCase::ReceivePdDataIndication(uint32_t psduLength,
                                                    Ptr<Packet> p,
                                                    uint8_t lqi,
                                                    int8_t rssi)
{
    NS_LOG_UNCOND("At: " << Simulator::Now() << " Received frame size: " << psduLength
                         << " LQI: " << static_cast<uint16_t>(lqi)
                         << " RSSI: " << static_cast<int16_t>(rssi) << " dBm.");

    m_receivedPackets++;
    m_receivedRssi = rssi;
}

void
LrWpanPhyCollisionTestCase::PacketCollisionDrop(Ptr<const Packet> p)
{
    m_packetDroppedByCollision++;
}

void
LrWpanPhyCollisionTestCase::DoRun()
{
    // Test the reaction of the PHY to receiving two packets simultaneously.
    // Since the receiver device will be in BUSY_RX state after the first packet
    // the 2nd packet will not be received. The first packet should be received correctly and
    // the second packet should be dropped due to collision and registered
    // in the drop trace (PhyRxDrop).
    // The second packet also affects the received RSSI
    // of the first packet since more energy will be present at the moment of receiving the packet.
    // This change on energy is also tested.

    LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC));
    LogComponentEnable("LrWpanPhy", LOG_LEVEL_ALL);

    Ptr<LrWpanPhy> sender = CreateObject<LrWpanPhy>();
    Ptr<LrWpanPhy> sender2 = CreateObject<LrWpanPhy>();
    Ptr<LrWpanPhy> receiver = CreateObject<LrWpanPhy>();

    // Add the SpectrumPhy to a channel and set the initial state of the PHYs
    // This is typically taken care by the NetDevice but in this case we are using the PHY directly.
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    sender->SetChannel(channel);
    sender2->SetChannel(channel);
    receiver->SetChannel(channel);
    channel->AddRx(receiver);

    sender->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_TX_ON);
    sender2->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_TX_ON);
    receiver->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);

    // Set the callback to receive packets at the receiver PHY
    receiver->SetPdDataIndicationCallback(
        MakeCallback(&LrWpanPhyCollisionTestCase::ReceivePdDataIndication, this));

    // Connect to trace source to log when a packet is dropped due to collision
    receiver->TraceConnectWithoutContext(
        "PhyRxDrop",
        MakeCallback(&LrWpanPhyCollisionTestCase::PacketCollisionDrop, this));

    Ptr<Packet> p = Create<Packet>(10); // 10 bytes
    Ptr<Packet> p2 = Create<Packet>(9); // 9 bytes
    Simulator::Schedule(Seconds(2), &LrWpanPhy::PdDataRequest, sender, p->GetSize(), p);
    Simulator::Schedule(Seconds(2), &LrWpanPhy::PdDataRequest, sender2, p2->GetSize(), p2);

    Simulator::Stop(Seconds(5));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(m_receivedPackets,
                          1,
                          "Error,the receiver should only be able to receive the first packet");

    NS_TEST_ASSERT_MSG_EQ(m_packetDroppedByCollision,
                          1,
                          "Error, one packet should be dropped due to collision");

    NS_TEST_ASSERT_MSG_GT(m_receivedRssi, 0, "Error,the received RSSI should be more than 0");

    Simulator::Destroy();
}

/**
 * @ingroup lr-wpan-test
 * @ingroup tests
 *
 * @brief LrWpan PLME and PD Interfaces TestSuite
 */
class LrWpanPlmeAndPdInterfaceTestSuite : public TestSuite
{
  public:
    LrWpanPlmeAndPdInterfaceTestSuite();
};

LrWpanPlmeAndPdInterfaceTestSuite::LrWpanPlmeAndPdInterfaceTestSuite()
    : TestSuite("lr-wpan-phy-test", Type::UNIT)
{
    // AddTestCase(new LrWpanPlmeAndPdInterfaceTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LrWpanPhyCollisionTestCase, TestCase::Duration::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static LrWpanPlmeAndPdInterfaceTestSuite
    g_lrWpanPlmeAndPdInterfaceTestSuite; //!< Static variable for test initialization
