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
#include "ns3/double.h"
#include "ns3/mgt-headers.h"
#include "ns3/mobility-helper.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/test.h"
#include "ns3/wifi-helper.h"
#include "ns3/wifi-mac-helper.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiP2pTest");

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the beaconing operation in IBSS.
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
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPower the TX power
     */
    virtual void Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, Watt_u txPower);

    /**
     * Update the BeaconJitter of the IBSS STAs based on the sender of the Beacon
     * to have control over the randomness of the Beacon transmission time. If the sender is the
     * same as the STA, the BeaconJitter is set to 1, otherwise it is set to 0.
     *
     * @param sender the address of the STA that just transmitted a Beacon
     */
    void UpdateBeaconJitters(Mac48Address sender);

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

    std::vector<BeaconFrameInfo> m_txBeacons;                 ///< transmitted beacons
    std::vector<Ptr<AdhocWifiMac>> m_macs;                    ///< MAC of the IBSS STAs
    std::vector<Ptr<ConstantRandomVariable>> m_beaconJitters; ///< BeaconJitter of the IBSS STAs
};

IbssBeaconingTest::IbssBeaconingTest()
    : TestCase("Check beaconing operation in IBSS")
{
}

void
IbssBeaconingTest::Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, Watt_u txPower)
{
    const auto& mpdu = *psduMap.cbegin()->second->begin();
    NS_LOG_FUNCTION(this << *mpdu << txVector << txPower);
    if (mpdu->GetHeader().IsBeacon())
    {
        MgtBeaconHeader beacon;
        mpdu->GetPacket()->PeekHeader(beacon);
        m_txBeacons.push_back({mpdu->GetHeader(), beacon});
        UpdateBeaconJitters(mpdu->GetHeader().GetAddr2());
    }
}

void
IbssBeaconingTest::UpdateBeaconJitters(Mac48Address sender)
{
    NS_LOG_FUNCTION(this << sender);
    for (std::size_t i = 0; i < m_macs.size(); ++i)
    {
        const auto value = (m_macs[i]->GetAddress() == sender) ? 1.0 : 0.0;
        m_beaconJitters[i]->SetAttribute("Constant", DoubleValue(value));
    }
}

void
IbssBeaconingTest::CheckResults()
{
    NS_TEST_EXPECT_MSG_EQ(m_txBeacons.size(), 10, "Expected 10 transmitted beacons");
    for (std::size_t i = 0; i < m_txBeacons.size(); ++i)
    {
        NS_TEST_EXPECT_MSG_EQ(m_txBeacons[i].header.GetAddr2(),
                              m_macs.at(i % 2)->GetAddress(),
                              "Expected STAs to alternate Beacon transmissions");
        const auto& capabilities = m_txBeacons[i].beacon.m_capability;
        NS_TEST_EXPECT_MSG_EQ(capabilities.IsEss(), false, "ESS bit should not be set for IBSS");
        NS_TEST_EXPECT_MSG_EQ(capabilities.IsIbss(), true, "IBSS bit should be set");
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
                BooleanValue(true),
                "EnableBeaconJitter",
                BooleanValue(false));

    auto ibssDevices = wifi.Install(phy, mac, ibssNodes);

    WifiHelper::AssignStreams(ibssDevices, streamNumber);

    for (uint32_t i = 0; i < ibssDevices.GetN(); ++i)
    {
        auto adhocMac =
            DynamicCast<AdhocWifiMac>(DynamicCast<WifiNetDevice>(ibssDevices.Get(i))->GetMac());
        auto jitter = CreateObject<ConstantRandomVariable>();
        jitter->SetAttribute("Constant", DoubleValue((i == 0) ? 0.0 : 1.0));
        adhocMac->SetAttribute("BeaconJitter", PointerValue(jitter));
        m_macs.push_back(adhocMac);
        m_beaconJitters.push_back(jitter);
    }

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
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi P2P test suite
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
