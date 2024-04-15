/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/adhoc-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/mgt-headers.h"
#include "ns3/mobility-helper.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/test.h"
#include "ns3/wifi-helper.h"
#include "ns3/wifi-mac-helper.h"
#include "ns3/wifi-psdu.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiP2pTest");

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test the beaconing operation in IBSS.
 */
class IbssBeaconingTest : public TestCase
{
  public:
    /**
     * Constructor
     */
    IbssBeaconingTest();

  private:
    void DoSetup() override;
    void DoRun() override;

    /**
     * Callback invoked when a FEM passes PSDUs to the PHY.
     *
     * \param psduMap the PSDU map
     * \param txVector the TX vector
     * \param txPowerW the tx power in Watts
     */
    virtual void Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW);

    /**
     * Check correctness of transmitted beacon frames
     */
    void CheckResults();

    /// Information about transmitted beacon frames
    struct BeaconFrameInfo
    {
        WifiMacHeader header;   ///< MAC header
        MgtBeaconHeader beacon; ///< Beacon header
    };

    std::vector<BeaconFrameInfo> m_txBeacons; ///< transmitted beacons
};

IbssBeaconingTest::IbssBeaconingTest()
    : TestCase("Check beaconing operation in IBSS")
{
}

void
IbssBeaconingTest::Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW)
{
    const auto& mpdu = *psduMap.cbegin()->second->begin();
    NS_LOG_FUNCTION(this << *mpdu << txVector << txPowerW);
    if (mpdu->GetHeader().IsBeacon())
    {
        MgtBeaconHeader beacon;
        mpdu->GetPacket()->PeekHeader(beacon);
        m_txBeacons.push_back({mpdu->GetHeader(), beacon});
    }
}

void
IbssBeaconingTest::CheckResults()
{
    NS_TEST_EXPECT_MSG_EQ(m_txBeacons.size(), 20, "Expected 20 transmitted beacons");
    std::map<Mac48Address, std::size_t> beaconsPerSta{};
    for (const auto& txBeacon : m_txBeacons)
    {
        const auto addr2 = txBeacon.header.GetAddr2();
        if (beaconsPerSta.contains(addr2))
        {
            beaconsPerSta[addr2]++;
        }
        else
        {
            beaconsPerSta[addr2] = 1;
        }
        const auto& capabilities = txBeacon.beacon.Capabilities();
        NS_TEST_EXPECT_MSG_EQ(capabilities.IsEss(), false, "ESS bit should not be set for IBSS");
    }
    NS_TEST_EXPECT_MSG_EQ(beaconsPerSta.size(), 2, "Expected transmitted beacons from two STAs");
    for (const auto& [sender, count] : beaconsPerSta)
    {
        NS_TEST_EXPECT_MSG_EQ(count, 10, "Expected 10 transmitted beacons from " << sender);
    }
}

void
IbssBeaconingTest::DoSetup()
{
    // WifiHelper::EnableLogComponents();
    // LogComponentEnable("WifiP2pTest", LOG_LEVEL_ALL);

    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 100;

    NodeContainer ibssNodes;
    ibssNodes.Create(2);

    auto channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ax);

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac",
                "Ssid",
                SsidValue(Ssid("ibss-ssid")),
                "BeaconGeneration",
                BooleanValue(true));

    auto ibssDevices = wifi.Install(phy, mac, ibssNodes);

    WifiHelper::AssignStreams(ibssDevices, streamNumber);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(ibssNodes);

    Config::ConnectWithoutContext(
        "/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Phys/0/PhyTxPsduBegin",
        MakeCallback(&IbssBeaconingTest::Transmit, this));
}

void
IbssBeaconingTest::DoRun()
{
    Simulator::Stop(Seconds(1));
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi P2P test suite
 */
class WifiP2pTestSuite : public TestSuite
{
  public:
    WifiP2pTestSuite();
};

WifiP2pTestSuite::WifiP2pTestSuite()
    : TestSuite("wifi-p2p", Type::UNIT)
{
    AddTestCase(new IbssBeaconingTest(), TestCase::Duration::QUICK);
}

static WifiP2pTestSuite g_wifiP2pTestSuite; ///< the test suite
