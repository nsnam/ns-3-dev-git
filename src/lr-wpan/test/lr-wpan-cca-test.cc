/*
 * Copyright (c) 2014 Fraunhofer FKIE
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Sascha Alexander Jopen <jopen@cs.uni-bonn.de>
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "ns3/constant-position-mobility-model.h"
#include "ns3/core-module.h"
#include "ns3/log.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/packet.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/single-model-spectrum-channel.h"

#include <iomanip>
#include <iostream>

using namespace ns3;
using namespace ns3::lrwpan;

NS_LOG_COMPONENT_DEFINE("lr-wpan-cca-test");

/**
 * @ingroup lr-wpan-test
 * @ingroup tests
 *
 * LrWpan CCA Test
 */
class LrWpanCcaTestCase : public TestCase
{
  public:
    LrWpanCcaTestCase();

  private:
    /**
     * Function called when PlmeCcaConfirm is hit.
     *
     * @param testcase The TestCase.
     * @param device The LrWpanNetDevice.
     * @param status The device status.
     */
    static void PlmeCcaConfirm(LrWpanCcaTestCase* testcase,
                               Ptr<LrWpanNetDevice> device,
                               PhyEnumeration status);
    /**
     * Function called when PhyTxBegin is hit.
     *
     * @param testcase The TestCase.
     * @param device The LrWpanNetDevice.
     * @param packet The packet.
     */
    static void PhyTxBegin(LrWpanCcaTestCase* testcase,
                           Ptr<LrWpanNetDevice> device,
                           Ptr<const Packet> packet);
    /**
     * Function called when PhyTxEnd is hit.
     *
     * @param testcase The TestCase.
     * @param device The LrWpanNetDevice.
     * @param packet The packet.
     */
    static void PhyTxEnd(LrWpanCcaTestCase* testcase,
                         Ptr<LrWpanNetDevice> device,
                         Ptr<const Packet> packet);
    /**
     * Function called when PhyRxBegin is hit.
     *
     * @param testcase The TestCase.
     * @param device The LrWpanNetDevice.
     * @param packet The packet.
     */
    static void PhyRxBegin(LrWpanCcaTestCase* testcase,
                           Ptr<LrWpanNetDevice> device,
                           Ptr<const Packet> packet);
    /**
     * Function called when PhyRxEnd is hit.
     *
     * @param testcase The TestCase.
     * @param device The LrWpanNetDevice.
     * @param packet The packet.
     * @param sinr The received SINR.
     */
    static void PhyRxEnd(LrWpanCcaTestCase* testcase,
                         Ptr<LrWpanNetDevice> device,
                         Ptr<const Packet> packet,
                         double sinr);
    /**
     * Function called when PhyRxDrop is hit.
     *
     * @param testcase The TestCase.
     * @param device The LrWpanNetDevice.
     * @param packet The packet.
     */
    static void PhyRxDrop(LrWpanCcaTestCase* testcase,
                          Ptr<LrWpanNetDevice> device,
                          Ptr<const Packet> packet);

    void DoRun() override;

    PhyEnumeration m_status; //!< PHY status.
};

LrWpanCcaTestCase::LrWpanCcaTestCase()
    : TestCase("Test the 802.15.4 clear channel assessment")
{
    m_status = IEEE_802_15_4_PHY_UNSPECIFIED;
}

void
LrWpanCcaTestCase::PlmeCcaConfirm(LrWpanCcaTestCase* testcase,
                                  Ptr<LrWpanNetDevice> device,
                                  PhyEnumeration status)
{
    std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(9) << "["
              << Simulator::Now().As(Time::S) << "] " << device->GetMac()->GetShortAddress()
              << " PlmeCcaConfirm: " << LrWpanHelper::LrWpanPhyEnumerationPrinter(status)
              << std::endl;

    testcase->m_status = status;
}

void
LrWpanCcaTestCase::PhyTxBegin(LrWpanCcaTestCase* testcase,
                              Ptr<LrWpanNetDevice> device,
                              Ptr<const Packet> packet)
{
    std::ostringstream os;
    packet->Print(os);
    std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(9) << "["
              << Simulator::Now().As(Time::S) << "] " << device->GetMac()->GetShortAddress()
              << " PhyTxBegin: " << os.str() << std::endl;
}

void
LrWpanCcaTestCase::PhyTxEnd(LrWpanCcaTestCase* testcase,
                            Ptr<LrWpanNetDevice> device,
                            Ptr<const Packet> packet)
{
    std::ostringstream os;
    packet->Print(os);
    std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(9) << "["
              << Simulator::Now().As(Time::S) << "] " << device->GetMac()->GetShortAddress()
              << " PhyTxEnd: " << os.str() << std::endl;
}

void
LrWpanCcaTestCase::PhyRxBegin(LrWpanCcaTestCase* testcase,
                              Ptr<LrWpanNetDevice> device,
                              Ptr<const Packet> packet)
{
    std::ostringstream os;
    packet->Print(os);
    std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(9) << "["
              << Simulator::Now().As(Time::S) << "] " << device->GetMac()->GetShortAddress()
              << " PhyRxBegin: " << os.str() << std::endl;
}

void
LrWpanCcaTestCase::PhyRxEnd(LrWpanCcaTestCase* testcase,
                            Ptr<LrWpanNetDevice> device,
                            Ptr<const Packet> packet,
                            double sinr)
{
    std::ostringstream os;
    packet->Print(os);
    std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(9) << "["
              << Simulator::Now().As(Time::S) << "] " << device->GetMac()->GetShortAddress()
              << " PhyRxEnd (" << sinr << "): " << os.str() << std::endl;

    // The first packet was received. Now start a CCA, to try to detect the second packet which is
    // still being transmitted.
    device->GetPhy()->PlmeCcaRequest();
}

void
LrWpanCcaTestCase::PhyRxDrop(LrWpanCcaTestCase* testcase,
                             Ptr<LrWpanNetDevice> device,
                             Ptr<const Packet> packet)
{
    std::ostringstream os;
    packet->Print(os);
    std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(9) << "["
              << Simulator::Now().As(Time::S) << "] " << device->GetMac()->GetShortAddress()
              << " PhyRxDrop: " << os.str() << std::endl;
}

void
LrWpanCcaTestCase::DoRun()
{
    // Tx Power: 0 dBm
    // Receiver Sensitivity: -106.58 dBm
    // CCA channel busy condition: Rx power > -96.58 dBm
    // Log distance reference loss at 1 m distance for channel 11 (2405 MHz): 40.0641 dB
    // Log distance free space path loss exponent: 2

    // Test setup:
    // Start transmission of a short packet from node 0 to node 1 and at the same
    // time a transmission of a large packet from node 2 to node 1.
    // Both transmissions should start without backoff (per configuration) because
    // the CCA on both nodes should detect a free medium.
    // The shorter packet will be received first. After reception of the short
    // packet, which might be destroyed due to interference of the large
    // packet, node 1 will start a CCA. Depending on the distance between node 1
    // and node 2, node 1 should detect a busy medium, because the transmission of
    // the large packet is still in progress. For the above mentioned scenario
    // parameters, the distance for the CCA detecting a busy medium is about
    // 669.5685 m.

    // Enable calculation of FCS in the trailers. Only necessary when interacting with real devices
    // or wireshark. GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

    // Set the random seed and run number for this test
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(6);

    // Create 3 nodes, and a NetDevice for each one
    Ptr<Node> n0 = CreateObject<Node>();
    Ptr<Node> n1 = CreateObject<Node>();
    Ptr<Node> n2 = CreateObject<Node>();

    Ptr<LrWpanNetDevice> dev0 = CreateObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev1 = CreateObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev2 = CreateObject<LrWpanNetDevice>();

    // Make random variable stream assignment deterministic
    dev0->AssignStreams(0);
    dev1->AssignStreams(10);
    dev2->AssignStreams(20);

    dev0->SetAddress(Mac16Address("00:01"));
    dev1->SetAddress(Mac16Address("00:02"));
    dev2->SetAddress(Mac16Address("00:03"));

    // Each device must be attached to the same channel
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    propModel->SetReference(1.0,
                            40.0641);  // Reference loss at 1m distance for 2405 MHz (channel 11)
    propModel->SetPathLossExponent(2); // Free space path loss exponent
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    dev0->SetChannel(channel);
    dev1->SetChannel(channel);
    dev2->SetChannel(channel);

    // To complete configuration, a LrWpanNetDevice must be added to a node
    n0->AddDevice(dev0);
    n1->AddDevice(dev1);
    n2->AddDevice(dev2);

    Ptr<ConstantPositionMobilityModel> sender0Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    sender0Mobility->SetPosition(Vector(0, 0, 0));
    dev0->GetPhy()->SetMobility(sender0Mobility);
    Ptr<ConstantPositionMobilityModel> sender1Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    sender1Mobility->SetPosition(Vector(0, 669, 0));
    dev1->GetPhy()->SetMobility(sender1Mobility);
    Ptr<ConstantPositionMobilityModel> sender2Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    sender2Mobility->SetPosition(Vector(0, 1338, 0));
    dev2->GetPhy()->SetMobility(sender2Mobility);

    // Disable the NetDevices queue management.
    dev0->GetMac()->SetMcpsDataConfirmCallback(MakeNullCallback<void, McpsDataConfirmParams>());
    dev1->GetMac()->SetMcpsDataConfirmCallback(MakeNullCallback<void, McpsDataConfirmParams>());
    dev2->GetMac()->SetMcpsDataConfirmCallback(MakeNullCallback<void, McpsDataConfirmParams>());

    // Set the CCA confirm callback.
    dev1->GetPhy()->SetPlmeCcaConfirmCallback(
        MakeBoundCallback(&LrWpanCcaTestCase::PlmeCcaConfirm, this, dev1));

    // Start sending without backoff, if the channel is free.
    dev0->GetCsmaCa()->SetMacMinBE(0);
    dev2->GetCsmaCa()->SetMacMinBE(0);

    // Connect trace sources.
    dev0->GetPhy()->TraceConnectWithoutContext(
        "PhyTxBegin",
        MakeBoundCallback(&LrWpanCcaTestCase::PhyTxBegin, this, dev0));
    dev0->GetPhy()->TraceConnectWithoutContext(
        "PhyTxEnd",
        MakeBoundCallback(&LrWpanCcaTestCase::PhyTxEnd, this, dev0));
    dev2->GetPhy()->TraceConnectWithoutContext(
        "PhyTxBegin",
        MakeBoundCallback(&LrWpanCcaTestCase::PhyTxBegin, this, dev2));
    dev2->GetPhy()->TraceConnectWithoutContext(
        "PhyTxEnd",
        MakeBoundCallback(&LrWpanCcaTestCase::PhyTxEnd, this, dev2));
    dev1->GetPhy()->TraceConnectWithoutContext(
        "PhyRxBegin",
        MakeBoundCallback(&LrWpanCcaTestCase::PhyRxBegin, this, dev1));
    dev1->GetPhy()->TraceConnectWithoutContext(
        "PhyRxEnd",
        MakeBoundCallback(&LrWpanCcaTestCase::PhyRxEnd, this, dev1));
    dev1->GetPhy()->TraceConnectWithoutContext(
        "PhyRxDrop",
        MakeBoundCallback(&LrWpanCcaTestCase::PhyRxDrop, this, dev1));

    m_status = IEEE_802_15_4_PHY_UNSPECIFIED;

    Ptr<Packet> p0 = Create<Packet>(1); // 1 byte of dummy data
    McpsDataRequestParams params0;
    params0.m_srcAddrMode = SHORT_ADDR;
    params0.m_dstAddrMode = SHORT_ADDR;
    params0.m_dstPanId = 0;
    params0.m_dstAddr = Mac16Address("00:02");
    params0.m_msduHandle = 0;
    params0.m_txOptions = TX_OPTION_NONE;
    Simulator::ScheduleNow(&LrWpanMac::McpsDataRequest, dev0->GetMac(), params0, p0);

    Ptr<Packet> p1 = Create<Packet>(100); // 100 bytes of dummy data
    McpsDataRequestParams params1;
    params1.m_srcAddrMode = SHORT_ADDR;
    params1.m_dstAddrMode = SHORT_ADDR;
    params1.m_dstPanId = 0;
    params1.m_dstAddr = Mac16Address("00:02");
    params1.m_msduHandle = 0;
    params1.m_txOptions = TX_OPTION_NONE;
    Simulator::ScheduleNow(&LrWpanMac::McpsDataRequest, dev2->GetMac(), params1, p1);

    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_status, IEEE_802_15_4_PHY_BUSY, "CCA status BUSY (as expected)");

    m_status = IEEE_802_15_4_PHY_UNSPECIFIED;

    sender2Mobility->SetPosition(Vector(0, 1340, 0));

    Simulator::ScheduleNow(&LrWpanMac::McpsDataRequest, dev0->GetMac(), params0, p0);
    Simulator::ScheduleNow(&LrWpanMac::McpsDataRequest, dev2->GetMac(), params1, p1);

    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_status, IEEE_802_15_4_PHY_IDLE, "CCA status IDLE (as expected)");

    Simulator::Destroy();
}

/**
 * @ingroup lr-wpan-test
 * @ingroup tests
 *
 * @brief Test the sensitivity of the CSMA/CA clear channel assestment (CCA)
 */
class CCAVulnerableWindowTest : public TestCase
{
  public:
    CCAVulnerableWindowTest();
    ~CCAVulnerableWindowTest() override;

  private:
    /**
     * Function called when a Data indication is invoked
     *
     * @param params MCPS data indication parameters
     * @param p packet
     */
    void DataIndication(McpsDataIndicationParams params, Ptr<Packet> p);

    void DoRun() override;

    uint8_t countPackets; //!< Count the number of packets received in the test
};

CCAVulnerableWindowTest::CCAVulnerableWindowTest()
    : TestCase("Test CCA false positives vulnerable window caused by turnAround")
{
}

CCAVulnerableWindowTest::~CCAVulnerableWindowTest()
{
}

void
CCAVulnerableWindowTest::DataIndication(McpsDataIndicationParams params, Ptr<Packet> p)
{
    countPackets++;
}

void
CCAVulnerableWindowTest::DoRun()
{
    /*
     *  CCA Vulnerable Window:
     *
     *  Topology
     *         Node 0             Node 1
     *         (dev 0) <--------  (dev1)
     *               ^
     *               |
     *               |------Node2
     *                    (dev2)
     *
     *  Transmission of packets over time:
     *
     *             Node1 CCA |  Node 1 TurnAround Rx->Tx | Node1 Packet Tx
     *                128us  |       192us               |
     *                                                                             = Collision
     *                   Node2 CCA |  Node 2 TurnAround Rx->Tx | Node2 Packet Tx
     *                      128us  |       192us               |
     * Time ---------------------------------------------------------------->
     *
     *
     *  If 2 packets are transmitted within 192 us from each other, CSMA/CA will give a false
     * positive and cause a collision even with CCA active. This is because there is a vulnerable
     * window due to the turn around (RX_ON->TX_ON) between the CCA and the actual transmission of
     * the packet. During this time, the 2nd node's CCA will assume the channel IDLE because there
     * is no transmission (the first node is in its turnaround before transmission).
     *
     *  To demonstrate this, In this test, Node 1 transmits a packet to Node 0 at second 1.
     *  Node 0 Attempts transmitting a packet to Node 2 at second and 1.000128 which falls under the
     * vulnerable window (1 and 1.000192 seconds). A collision occurs and both packets failed to be
     * received. All 3 nodes are within each other communication range and csma/ca backoff periods
     * are turn off to make this test reprodusable.
     *
     *  The test is repeated but outside the vulnerable window, in this case CSMA/CA works as
     * intended: The CCA of node 2 detects the first packet in the medium and defers the
     * transmission. Avoiding effectively the collision.
     */

    LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC | LOG_PREFIX_NODE));
    LogComponentEnable("LrWpanMac", LOG_LEVEL_DEBUG);

    // Create 3 nodes, and a NetDevice for each one
    Ptr<Node> n0 = CreateObject<Node>();
    Ptr<Node> n1 = CreateObject<Node>();
    Ptr<Node> n2 = CreateObject<Node>();

    Ptr<LrWpanNetDevice> dev0 = CreateObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev1 = CreateObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev2 = CreateObject<LrWpanNetDevice>();

    // Make random variable stream assignment deterministic
    dev0->AssignStreams(0);
    dev1->AssignStreams(10);
    dev2->AssignStreams(20);

    dev0->GetMac()->SetExtendedAddress("00:00:00:00:00:00:CA:FE");
    dev1->GetMac()->SetExtendedAddress("00:00:00:00:00:00:00:01");
    dev2->GetMac()->SetExtendedAddress("00:00:00:00:00:00:00:02");

    dev0->SetAddress(Mac16Address("00:00"));
    dev1->SetAddress(Mac16Address("00:01"));
    dev2->SetAddress(Mac16Address("00:02"));

    // Note: By default all devices are in the PANID: 0

    // Each device must be attached to the same channel
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    dev0->SetChannel(channel);
    dev1->SetChannel(channel);
    dev2->SetChannel(channel);

    // To complete configuration, a LrWpanNetDevice must be added to a node
    n0->AddDevice(dev0);
    n1->AddDevice(dev1);
    n2->AddDevice(dev2);

    // Set mobility
    Ptr<ConstantPositionMobilityModel> dev0Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev0Mobility->SetPosition(Vector(0, 0, 0));
    dev0->GetPhy()->SetMobility(dev0Mobility);

    Ptr<ConstantPositionMobilityModel> dev1Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev1Mobility->SetPosition(Vector(10, 0, 0));
    dev1->GetPhy()->SetMobility(dev1Mobility);

    Ptr<ConstantPositionMobilityModel> dev2Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev2Mobility->SetPosition(Vector(5, 5, 0));
    dev2->GetPhy()->SetMobility(dev2Mobility);

    // Do a CCA without a previous RandomBackoffDelay
    // This is set to make the test reprodusable.
    dev0->GetCsmaCa()->SetMacMinBE(0);
    dev1->GetCsmaCa()->SetMacMinBE(0);
    dev2->GetCsmaCa()->SetMacMinBE(0);

    // Callback hooks to MAC layer
    dev0->GetMac()->SetMcpsDataIndicationCallback(
        MakeCallback(&CCAVulnerableWindowTest::DataIndication, this));

    dev1->GetMac()->SetMcpsDataIndicationCallback(
        MakeCallback(&CCAVulnerableWindowTest::DataIndication, this));

    dev2->GetMac()->SetMcpsDataIndicationCallback(
        MakeCallback(&CCAVulnerableWindowTest::DataIndication, this));

    // --------------------------------------------------------------------------
    /// Outside the vulnerable window, 2 packets are meant to be received
    countPackets = 0;

    // Device 1 send data to Device 0
    Ptr<Packet> p0 = Create<Packet>(50);
    McpsDataRequestParams params;
    params.m_dstPanId = 0;
    params.m_srcAddrMode = SHORT_ADDR;
    params.m_dstAddrMode = SHORT_ADDR;
    params.m_dstAddr = Mac16Address("00:00");
    params.m_msduHandle = 0;
    Simulator::ScheduleWithContext(1,
                                   Seconds(1),
                                   &LrWpanMac::McpsDataRequest,
                                   dev1->GetMac(),
                                   params,
                                   p0);

    // Device 0 send data to Device 2
    Ptr<Packet> p1 = Create<Packet>(50);
    McpsDataRequestParams params1;
    params1.m_dstPanId = 0;
    params1.m_srcAddrMode = SHORT_ADDR;
    params1.m_dstAddrMode = SHORT_ADDR;
    params1.m_dstAddr = Mac16Address("00:02");
    params1.m_msduHandle = 1;
    Simulator::ScheduleWithContext(1,
                                   Seconds(1.000193),
                                   &LrWpanMac::McpsDataRequest,
                                   dev0->GetMac(),
                                   params1,
                                   p1);

    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(countPackets, 2, "2 Packets are meant to be received");
    // --------------------------------------------------------------------------
    // Within the vulnerable window, a collion occurs and no
    // packets are meant to be received
    countPackets = 0;

    // Device 1 send data to Device 0
    Ptr<Packet> p3 = Create<Packet>(50);
    McpsDataRequestParams params3;
    params3.m_dstPanId = 0;
    params3.m_srcAddrMode = SHORT_ADDR;
    params3.m_dstAddrMode = SHORT_ADDR;
    params3.m_dstAddr = Mac16Address("00:00");
    params3.m_msduHandle = 0;
    Simulator::ScheduleWithContext(1,
                                   Seconds(1),
                                   &LrWpanMac::McpsDataRequest,
                                   dev1->GetMac(),
                                   params3,
                                   p3);

    // Device 0 send data to Device 2
    // This devices sends a second packet in less that 192us (turnaround time)
    // apart from the first packet transmission sent by Device 1
    Ptr<Packet> p4 = Create<Packet>(50);
    McpsDataRequestParams params4;
    params4.m_dstPanId = 0;
    params4.m_srcAddrMode = SHORT_ADDR;
    params4.m_dstAddrMode = SHORT_ADDR;
    params4.m_dstAddr = Mac16Address("00:02");
    params4.m_msduHandle = 1;
    Simulator::ScheduleWithContext(1,
                                   Seconds(1.000121),
                                   &LrWpanMac::McpsDataRequest,
                                   dev0->GetMac(),
                                   params4,
                                   p4);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(countPackets,
                          0,
                          "Collision is meant to happen and no packets should be received");

    Simulator::Destroy();
}

/**
 * @ingroup lr-wpan-test
 * @ingroup tests
 *
 * LrWpan ACK TestSuite
 */
class LrWpanCcaTestSuite : public TestSuite
{
  public:
    LrWpanCcaTestSuite();
};

LrWpanCcaTestSuite::LrWpanCcaTestSuite()
    : TestSuite("lr-wpan-cca-test", Type::UNIT)
{
    AddTestCase(new LrWpanCcaTestCase, TestCase::Duration::QUICK);
    AddTestCase(new CCAVulnerableWindowTest, TestCase::Duration::QUICK);
}

static LrWpanCcaTestSuite g_lrWpanCcaTestSuite; //!< Static variable for test initialization
