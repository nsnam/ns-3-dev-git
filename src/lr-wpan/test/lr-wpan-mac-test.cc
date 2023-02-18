/*
 * Copyright (c) 2022 Tokushima University, Japan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include <ns3/constant-position-mobility-model.h>
#include <ns3/core-module.h>
#include <ns3/log.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/packet.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>

#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("lr-wpan-mac-test");

/**
 * \ingroup lr-wpan-test
 * \ingroup tests
 *
 * \brief Test PHY going to TRX_OFF after CSMA failure (MAC->RxOnWhenIdle(false))
 */
class TestRxOffWhenIdleAfterCsmaFailure : public TestCase
{
  public:
    TestRxOffWhenIdleAfterCsmaFailure();
    ~TestRxOffWhenIdleAfterCsmaFailure() override;

  private:
    /**
     * Function called when a Data indication is invoked
     * \param params MCPS data indication parameters
     * \param p packet
     */
    void DataIndication(McpsDataIndicationParams params, Ptr<Packet> p);
    /**
     * Function called when a Data confirm is invoked (After Tx Attempt)
     * \param params MCPS data confirm parameters
     */
    void DataConfirm(McpsDataConfirmParams params);
    /**
     * Function called when a the PHY state changes in Dev0 [00:01]
     * \param context context
     * \param now time at which the function is called
     * \param oldState old PHY state
     * \param newState new PHY state
     */
    void StateChangeNotificationDev0(std::string context,
                                     Time now,
                                     LrWpanPhyEnumeration oldState,
                                     LrWpanPhyEnumeration newState);
    /**
     * Function called when a the PHY state changes in Dev2 [00:03]
     * \param context context
     * \param now time at which the function is called
     * \param oldState old PHY state
     * \param newState new PHY state
     */
    void StateChangeNotificationDev2(std::string context,
                                     Time now,
                                     LrWpanPhyEnumeration oldState,
                                     LrWpanPhyEnumeration newState);

    void DoRun() override;

    LrWpanPhyEnumeration m_dev0State; //!< Stores the PHY state of device 0 [00:01]
};

TestRxOffWhenIdleAfterCsmaFailure::TestRxOffWhenIdleAfterCsmaFailure()
    : TestCase("Test PHY going to TRX_OFF state after CSMA failure")
{
}

TestRxOffWhenIdleAfterCsmaFailure::~TestRxOffWhenIdleAfterCsmaFailure()
{
}

void
TestRxOffWhenIdleAfterCsmaFailure::DataIndication(McpsDataIndicationParams params, Ptr<Packet> p)
{
    NS_LOG_DEBUG("Received packet of size " << p->GetSize());
}

void
TestRxOffWhenIdleAfterCsmaFailure::DataConfirm(McpsDataConfirmParams params)
{
    if (params.m_status == LrWpanMcpsDataConfirmStatus::IEEE_802_15_4_SUCCESS)
    {
        NS_LOG_DEBUG("LrWpanMcpsDataConfirmStatus = Success");
    }
    else if (params.m_status == LrWpanMcpsDataConfirmStatus::IEEE_802_15_4_CHANNEL_ACCESS_FAILURE)
    {
        NS_LOG_DEBUG("LrWpanMcpsDataConfirmStatus =  Channel Access Failure");
    }
}

void
TestRxOffWhenIdleAfterCsmaFailure::StateChangeNotificationDev0(std::string context,
                                                               Time now,
                                                               LrWpanPhyEnumeration oldState,
                                                               LrWpanPhyEnumeration newState)
{
    NS_LOG_DEBUG(Simulator::Now().As(Time::S)
                 << context << "PHY state change at " << now.As(Time::S) << " from "
                 << LrWpanHelper::LrWpanPhyEnumerationPrinter(oldState) << " to "
                 << LrWpanHelper::LrWpanPhyEnumerationPrinter(newState));

    m_dev0State = newState;
}

void
TestRxOffWhenIdleAfterCsmaFailure::StateChangeNotificationDev2(std::string context,
                                                               Time now,
                                                               LrWpanPhyEnumeration oldState,
                                                               LrWpanPhyEnumeration newState)
{
    NS_LOG_DEBUG(Simulator::Now().As(Time::S)
                 << context << "PHY state change at " << now.As(Time::S) << " from "
                 << LrWpanHelper::LrWpanPhyEnumerationPrinter(oldState) << " to "
                 << LrWpanHelper::LrWpanPhyEnumerationPrinter(newState));
}

void
TestRxOffWhenIdleAfterCsmaFailure::DoRun()
{
    //  [00:01]      [00:02]     [00:03]
    //   Node 0------>Node1<------Node2 (interferer)
    //
    // Test Setup:
    //
    // Start the test with a transmission from node 2 to node 1,
    // soon after, node 0 will attempt to transmit a packet to node 1 as well but
    // it will fail because node 2 is still transmitting.
    //
    // The test confirms that the PHY in node 0 goes to TRX_OFF
    // after its CSMA failure because its MAC has been previously
    // set with flag RxOnWhenIdle (false). To ensure that node 0 CSMA
    // do not attempt to do multiple backoffs delays in its CSMA,
    // macMinBE and MacMaxCSMABackoffs has been set to 0.

    LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC));

    LogComponentEnable("LrWpanMac", LOG_LEVEL_DEBUG);
    LogComponentEnable("LrWpanCsmaCa", LOG_LEVEL_DEBUG);
    LogComponentEnable("lr-wpan-mac-test", LOG_LEVEL_DEBUG);

    // Create 3 nodes, and a NetDevice for each one
    Ptr<Node> n0 = CreateObject<Node>();
    Ptr<Node> n1 = CreateObject<Node>();
    Ptr<Node> interferenceNode = CreateObject<Node>();

    Ptr<LrWpanNetDevice> dev0 = CreateObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev1 = CreateObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev2 = CreateObject<LrWpanNetDevice>();

    dev0->SetAddress(Mac16Address("00:01"));
    dev1->SetAddress(Mac16Address("00:02"));
    dev2->SetAddress(Mac16Address("00:03"));

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
    interferenceNode->AddDevice(dev2);

    // Trace state changes in the phy
    dev0->GetPhy()->TraceConnect(
        "TrxState",
        std::string("[address 00:01]"),
        MakeCallback(&TestRxOffWhenIdleAfterCsmaFailure::StateChangeNotificationDev0, this));
    dev2->GetPhy()->TraceConnect(
        "TrxState",
        std::string("[address 00:03]"),
        MakeCallback(&TestRxOffWhenIdleAfterCsmaFailure::StateChangeNotificationDev2, this));

    Ptr<ConstantPositionMobilityModel> sender0Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    sender0Mobility->SetPosition(Vector(0, 0, 0));
    dev0->GetPhy()->SetMobility(sender0Mobility);
    Ptr<ConstantPositionMobilityModel> sender1Mobility =
        CreateObject<ConstantPositionMobilityModel>();

    sender1Mobility->SetPosition(Vector(0, 1, 0));
    dev1->GetPhy()->SetMobility(sender1Mobility);

    Ptr<ConstantPositionMobilityModel> sender3Mobility =
        CreateObject<ConstantPositionMobilityModel>();

    sender3Mobility->SetPosition(Vector(0, 2, 0));
    dev2->GetPhy()->SetMobility(sender3Mobility);

    McpsDataConfirmCallback cb0;
    cb0 = MakeCallback(&TestRxOffWhenIdleAfterCsmaFailure::DataConfirm, this);
    dev0->GetMac()->SetMcpsDataConfirmCallback(cb0);

    McpsDataIndicationCallback cb1;
    cb1 = MakeCallback(&TestRxOffWhenIdleAfterCsmaFailure::DataIndication, this);
    dev0->GetMac()->SetMcpsDataIndicationCallback(cb1);

    McpsDataConfirmCallback cb2;
    cb2 = MakeCallback(&TestRxOffWhenIdleAfterCsmaFailure::DataConfirm, this);
    dev1->GetMac()->SetMcpsDataConfirmCallback(cb2);

    McpsDataIndicationCallback cb3;
    cb3 = MakeCallback(&TestRxOffWhenIdleAfterCsmaFailure::DataIndication, this);
    dev1->GetMac()->SetMcpsDataIndicationCallback(cb3);

    dev0->GetMac()->SetRxOnWhenIdle(false);
    dev1->GetMac()->SetRxOnWhenIdle(false);

    // set CSMA min beacon exponent (BE) to 0 to make all backoff delays == to 0 secs.
    dev0->GetCsmaCa()->SetMacMinBE(0);
    dev2->GetCsmaCa()->SetMacMinBE(0);

    // Only try once to do a backoff period before giving up.
    dev0->GetCsmaCa()->SetMacMaxCSMABackoffs(0);
    dev2->GetCsmaCa()->SetMacMaxCSMABackoffs(0);

    // The below should trigger two callbacks when end-to-end data is working
    // 1) DataConfirm callback is called
    // 2) DataIndication callback is called with value of 50
    Ptr<Packet> p0 = Create<Packet>(50); // 50 bytes of dummy data
    McpsDataRequestParams params;
    params.m_dstPanId = 0;

    params.m_srcAddrMode = SHORT_ADDR;
    params.m_dstAddrMode = SHORT_ADDR;
    params.m_dstAddr = Mac16Address("00:02");

    params.m_msduHandle = 0;

    // Send the packet that will be rejected due to channel access failure
    Simulator::ScheduleWithContext(1,
                                   Seconds(0.00033),
                                   &LrWpanMac::McpsDataRequest,
                                   dev0->GetMac(),
                                   params,
                                   p0);

    // Send interference packet
    Ptr<Packet> p2 = Create<Packet>(60); // 60 bytes of dummy data
    params.m_dstAddr = Mac16Address("00:02");

    Simulator::ScheduleWithContext(2,
                                   Seconds(0.0),
                                   &LrWpanMac::McpsDataRequest,
                                   dev2->GetMac(),
                                   params,
                                   p2);

    NS_LOG_DEBUG("----------- Start of TestRxOffWhenIdleAfterCsmaFailure -------------------");
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_dev0State,
                          LrWpanPhyEnumeration::IEEE_802_15_4_PHY_TRX_OFF,
                          "Error, dev0 [00:01] PHY should be in TRX_OFF after CSMA failure");

    Simulator::Destroy();
}

/**
 * \ingroup lr-wpan-test
 * \ingroup tests
 *
 * \brief Test MAC Active Scan PAN descriptor reception and check some of its values.
 */
class TestActiveScanPanDescriptors : public TestCase
{
  public:
    TestActiveScanPanDescriptors();
    ~TestActiveScanPanDescriptors() override;

  private:
    /**
     * Function called in response to a MAC scan request.
     *
     * \param params MLME scan confirm parameters
     */
    void ScanConfirm(MlmeScanConfirmParams params);
    /**
     * Function used to notify the reception of a beacon with payload.
     *
     * \param params The MLME-BEACON-NOTIFY.indication parameters
     */
    void BeaconNotifyIndication(MlmeBeaconNotifyIndicationParams params);

    void DoRun() override;

    std::vector<PanDescriptor> m_panDescriptorList; //!< The list of PAN descriptors
                                                    //!< accumulated during the scan.
    uint32_t g_beaconPayloadSize;                   //!< The size of the beacon payload received
                                                    //!< from a coordinator.
};

TestActiveScanPanDescriptors::TestActiveScanPanDescriptors()
    : TestCase("Test the reception of PAN descriptors while performing an active scan")
{
}

TestActiveScanPanDescriptors::~TestActiveScanPanDescriptors()
{
}

void
TestActiveScanPanDescriptors::ScanConfirm(MlmeScanConfirmParams params)
{
    if (params.m_status == MLMESCAN_SUCCESS)
    {
        m_panDescriptorList = params.m_panDescList;
    }
}

void
TestActiveScanPanDescriptors::BeaconNotifyIndication(MlmeBeaconNotifyIndicationParams params)
{
    g_beaconPayloadSize = params.m_sdu->GetSize();
}

void
TestActiveScanPanDescriptors::DoRun()
{
    /*
     *      [00:01]                       [00:02]                             [00:03]
     *  PAN Coordinator 1 (PAN: 5)       End Device                  PAN Coordinator 2 (PAN: 7)
     *
     *       |--------100 m----------------|----------106 m -----------------------|
     *  Channel 12               (Active Scan channels 11-14)                 Channel 14
     *
     * Test Setup:
     *
     * At the beginning of the simulation, PAN coordinators are set to
     * non-beacon enabled mode and wait for any beacon requests.
     *
     * During the simulation, the end device do an Active scan (i.e. send beacon request commands to
     * the scanned channels). On reception of such commands, coordinators reply with a single beacon
     * which contains a PAN descriptor. The test makes sure that the PAN descriptors are received (2
     * PAN descriptors) and because both PAN coordinators are set to a different distance from the
     * end device, their LQI values should be below 255 but above 0. Likewise, Coordinator 2 LQI
     * value should be less than Coordinator 1 LQI value. The exact expected value of LQI is not
     * tested, this is dependable on the LQI implementation.
     */

    // Create 2 PAN coordinator nodes, and 1 end device
    Ptr<Node> coord1 = CreateObject<Node>();
    Ptr<Node> endNode = CreateObject<Node>();
    Ptr<Node> coord2 = CreateObject<Node>();

    Ptr<LrWpanNetDevice> coord1NetDevice = CreateObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> endNodeNetDevice = CreateObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> coord2NetDevice = CreateObject<LrWpanNetDevice>();

    coord1NetDevice->SetAddress(Mac16Address("00:01"));
    endNodeNetDevice->SetAddress(Mac16Address("00:02"));
    coord2NetDevice->SetAddress(Mac16Address("00:03"));

    // Configure Spectrum channel
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    coord1NetDevice->SetChannel(channel);
    endNodeNetDevice->SetChannel(channel);
    coord2NetDevice->SetChannel(channel);

    coord1->AddDevice(coord1NetDevice);
    endNode->AddDevice(endNodeNetDevice);
    coord2->AddDevice(coord2NetDevice);

    // Mobility
    Ptr<ConstantPositionMobilityModel> coord1Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    coord1Mobility->SetPosition(Vector(0, 0, 0));
    coord1NetDevice->GetPhy()->SetMobility(coord1Mobility);

    Ptr<ConstantPositionMobilityModel> endNodeMobility =
        CreateObject<ConstantPositionMobilityModel>();
    endNodeMobility->SetPosition(Vector(100, 0, 0));
    endNodeNetDevice->GetPhy()->SetMobility(endNodeMobility);

    Ptr<ConstantPositionMobilityModel> coord2Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    coord2Mobility->SetPosition(Vector(206, 0, 0));
    coord2NetDevice->GetPhy()->SetMobility(coord2Mobility);

    // MAC layer Callbacks hooks
    MlmeScanConfirmCallback cb0;
    cb0 = MakeCallback(&TestActiveScanPanDescriptors::ScanConfirm, this);
    endNodeNetDevice->GetMac()->SetMlmeScanConfirmCallback(cb0);

    MlmeBeaconNotifyIndicationCallback cb1;
    cb1 = MakeCallback(&TestActiveScanPanDescriptors::BeaconNotifyIndication, this);
    endNodeNetDevice->GetMac()->SetMlmeBeaconNotifyIndicationCallback(cb1);

    /////////////////
    // ACTIVE SCAN //
    /////////////////

    // PAN coordinator N0 (PAN 5) is set to channel 12 in non-beacon mode but answer to beacon
    // requests.
    MlmeStartRequestParams params;
    params.m_panCoor = true;
    params.m_PanId = 5;
    params.m_bcnOrd = 15;
    params.m_sfrmOrd = 15;
    params.m_logCh = 12;
    Simulator::ScheduleWithContext(1,
                                   Seconds(2.0),
                                   &LrWpanMac::MlmeStartRequest,
                                   coord1NetDevice->GetMac(),
                                   params);

    // PAN coordinator N2 (PAN 7) is set to channel 14 in non-beacon mode but answer to beacon
    // requests. The second coordinator includes a beacon payload of 25 bytes using the
    // MLME-SET.request primitive.
    Ptr<LrWpanMacPibAttributes> pibAttribute = Create<LrWpanMacPibAttributes>();
    pibAttribute->macBeaconPayload = Create<Packet>(25);
    coord2NetDevice->GetMac()->MlmeSetRequest(LrWpanMacPibAttributeIdentifier::macBeaconPayload,
                                              pibAttribute);

    MlmeStartRequestParams params2;
    params2.m_panCoor = true;
    params2.m_PanId = 7;
    params2.m_bcnOrd = 15;
    params2.m_sfrmOrd = 15;
    params2.m_logCh = 14;
    Simulator::ScheduleWithContext(2,
                                   Seconds(2.0),
                                   &LrWpanMac::MlmeStartRequest,
                                   coord2NetDevice->GetMac(),
                                   params2);

    // End device N1 broadcast a single BEACON REQUEST for each channel (11, 12, 13, and 14).
    // If a coordinator is present and in range, it will respond with a beacon broadcast.
    // Scan Channels are represented by bits 0-26  (27 LSB)
    //                       ch 14  ch 11
    //                           |  |
    // 0x7800  = 0000000000000000111100000000000
    MlmeScanRequestParams scanParams;
    scanParams.m_chPage = 0;
    scanParams.m_scanChannels = 0x7800;
    scanParams.m_scanDuration = 14;
    scanParams.m_scanType = MLMESCAN_ACTIVE;
    Simulator::ScheduleWithContext(1,
                                   Seconds(3.0),
                                   &LrWpanMac::MlmeScanRequest,
                                   endNodeNetDevice->GetMac(),
                                   scanParams);

    Simulator::Stop(Seconds(2000));
    NS_LOG_DEBUG("----------- Start of TestActiveScanPanDescriptors -------------------");
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_panDescriptorList.size(),
                          2,
                          "Error, Beacons not received or PAN descriptors not found");
    NS_TEST_ASSERT_MSG_LT(m_panDescriptorList[0].m_linkQuality,
                          255,
                          "Error, Coordinator 1 (PAN 5) LQI value should be less than 255.");
    NS_TEST_ASSERT_MSG_LT(m_panDescriptorList[1].m_linkQuality,
                          255,
                          "Error, Coordinator 2 (PAN 7) LQI value should be less than 255.");
    NS_TEST_ASSERT_MSG_GT(m_panDescriptorList[0].m_linkQuality,
                          0,
                          "Error, Coordinator 1 (PAN 5) LQI value should be greater than 0.");
    NS_TEST_ASSERT_MSG_GT(m_panDescriptorList[1].m_linkQuality,
                          0,
                          "Error, Coordinator 2 (PAN 7) LQI value should be greater than 0.");

    NS_TEST_ASSERT_MSG_LT(
        m_panDescriptorList[1].m_linkQuality,
        m_panDescriptorList[0].m_linkQuality,
        "Error, Coordinator 2 (PAN 7) LQI value should be less than Coordinator 1 (PAN 5).");

    NS_TEST_EXPECT_MSG_EQ(g_beaconPayloadSize,
                          25,
                          "Error, Beacon Payload not received or incorrect size (25 bytes)");

    Simulator::Destroy();
}

/**
 * \ingroup lr-wpan-test
 * \ingroup tests
 *
 * \brief LrWpan MAC TestSuite
 */
class LrWpanMacTestSuite : public TestSuite
{
  public:
    LrWpanMacTestSuite();
};

LrWpanMacTestSuite::LrWpanMacTestSuite()
    : TestSuite("lr-wpan-mac-test", UNIT)
{
    AddTestCase(new TestRxOffWhenIdleAfterCsmaFailure, TestCase::QUICK);
    AddTestCase(new TestActiveScanPanDescriptors, TestCase::QUICK);
}

static LrWpanMacTestSuite g_lrWpanMacTestSuite; //!< Static variable for test initialization
