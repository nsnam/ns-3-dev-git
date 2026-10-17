/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Davide Magrin <davide@magr.in>
 */

#include "ns3/ap-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/ctrl-headers.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/node-container.h"
#include "ns3/node-list.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/packet.h"
#include "ns3/qos-txop.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/wifi-mac-helper.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-static-setup-helper.h"

#include <list>
#include <set>
#include <sstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiProtectionTest");

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test that a DL MU PPDU mixing protected and unprotected receivers gets an MU-RTS.
 *
 * With a non-zero TXOP limit, stations that responded to the AP earlier in a TXOP are
 * recorded as protected for the remainder of that TXOP, so a DL MU PPDU prepared for a
 * non-initial frame exchange of the TXOP can mix protected and unprotected receivers.
 * The MU-RTS decision is a property of the PPDU's receiver set (send an MU-RTS if any
 * receiver is unprotected), not of the single MPDU being aggregated.
 *
 * An AP transmits two DL MU PPDUs to three stations within the same TXOP. Packets for
 * the first DL MU PPDU (to STA 1 and STA 2) are generated at the beginning of the
 * simulation; packets for the second DL MU PPDU (to STA 2 and STA 3) are generated as
 * soon as the first DL MU PPDU is transmitted. The first MU-RTS/CTS exchange protects
 * STA 1 and STA 2 only, hence the second DL MU PPDU mixes a protected receiver (STA 2)
 * with an unprotected one (STA 3) and must be preceded by another MU-RTS.
 *
 * The expected frame exchange sequence is (all frames within one TXOP, hence
 * SIFS-separated; the second MU-RTS is required because STA 3 is not protected yet;
 * the CF-End truncates the TXOP once all queues are empty):
 *
 * @verbatim
 *
 * packets to STA 1, STA 2   packets to STA 2, STA 3
 *    queued at the AP         queued at this point
 *           │                │
 *           ▼                ▼
 *        ┌──────┐       ┌──────────┐  ┌──────┐
 * [AP]   │MU-RTS│       │DL MU PPDU│  │MU-BAR│
 * ───────┴──────┴─┬───┬─┴──────────┴──┴──────┴─┬──┬ ─ ─ ─
 * STA 1           │CTS│                        │BA│
 *                 └───┘                        └──┘
 * STA 2           │CTS│                        │BA│
 *                 └───┘                        └──┘
 *
 *  (the TXOP continues: the second MU-RTS follows a SIFS after the BlockAcks)
 *
 *        ┌──────┐       ┌──────────┐  ┌──────┐      ┌──────┐
 * [AP]   │MU-RTS│       │DL MU PPDU│  │MU-BAR│      │CF-End│
 * ───────┴──────┴─┬───┬─┴──────────┴──┴──────┴─┬──┬─┴──────┴────
 * STA 2           │CTS│                        │BA│
 *                 └───┘                        └──┘
 * STA 3           │CTS│                        │BA│
 *                 └───┘                        └──┘
 *
 * @endverbatim
 *
 * Association and Block Ack agreement establishment are performed through the static
 * setup helper and UL OFDMA is disabled on the multi-user scheduler, so that the frames
 * above are the only ones transmitted. The test checks the type of every transmitted
 * frame against this sequence, that both MU-RTS frames solicit both receivers of the
 * respective PPDU, and that all expected frames have been transmitted by the end of the
 * simulation.
 */
class MuRtsUnprotectedReceiverTest : public TestCase
{
  public:
    MuRtsUnprotectedReceiverTest();

    /**
     * Callback invoked when a PHY passes a PSDU map to transmit.
     *
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPower the TX power
     */
    void Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, Watt_u txPower);

  private:
    /// Expected start time of a frame relative to the previous frame in the sequence
    enum class ExpectedStart : uint8_t
    {
        ANY,        ///< no expectation (first frame of the sequence)
        AFTER_SIFS, ///< a SIFS after the end of the previous frame
        SAME_TIME   ///< at the same time as the previous frame (simultaneous responses)
    };

    /// Actions and checks to perform upon the transmission of each frame
    struct Events
    {
        /**
         * Constructor.
         *
         * @param type the frame MAC header type
         * @param s the expected start time of the frame
         * @param f function to perform actions and checks
         */
        Events(WifiMacType type,
               ExpectedStart s,
               std::function<void(const WifiConstPsduMap&, const WifiTxVector&)>&& f = {})
            : hdrType(type),
              start(s),
              func(f)
        {
        }

        WifiMacType hdrType; ///< MAC header type of the frame
        ExpectedStart start; ///< expected start time of the frame
        std::function<void(const WifiConstPsduMap&, const WifiTxVector&)>
            func; ///< actions and checks to perform
    };

    void DoSetup() override;
    void DoRun() override;

    /**
     * @param staId the index of the station the generated packets are destined to
     * @return an application generating one packet destined to the given station
     */
    Ptr<PacketSocketClient> GetApplication(std::size_t staId) const;

    /// Insert elements in the list of expected events (transmitted frames)
    void InsertEvents();

    /**
     * Check that the given PSDU is an MU-RTS soliciting exactly the given stations.
     *
     * @param psdu the PSDU carrying the Trigger Frame
     * @param staIds the indices of the solicited stations
     */
    void CheckMuRts(Ptr<const WifiPsdu> psdu, const std::set<std::size_t>& staIds);

    /**
     * Check that the given PSDU map is a DL MU PPDU addressed to exactly the given
     * stations.
     *
     * @param psduMap the PSDU map
     * @param staIds the indices of the receiver stations
     */
    void CheckDlMuPpdu(const WifiConstPsduMap& psduMap, const std::set<std::size_t>& staIds);

    /// number of contending stations
    static constexpr std::size_t N_STAS = 3;

    Ptr<ApWifiMac> m_apMac;                       ///< AP wifi MAC
    std::vector<Ptr<StaWifiMac>> m_staMacs;       ///< station wifi MACs
    std::vector<PacketSocketAddress> m_dlSockets; ///< packet socket addresses for DL traffic
    std::list<Events> m_events;                   ///< list of events for the test run
    Time m_prevTxStart;                           ///< start time of the previous frame
    Time m_prevTxEnd;                             ///< end time of the previous frame
    const Time m_duration{MilliSeconds(50)};      ///< simulation duration
    const Time m_txopLimit{MicroSeconds(4800)};   ///< TXOP limit for the AC of the DL traffic
};

MuRtsUnprotectedReceiverTest::MuRtsUnprotectedReceiverTest()
    : TestCase("Check that a DL MU PPDU mixing protected and unprotected receivers gets an MU-RTS")
{
}

Ptr<PacketSocketClient>
MuRtsUnprotectedReceiverTest::GetApplication(std::size_t staId) const
{
    auto client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(500));
    client->SetAttribute("MaxPackets", UintegerValue(1));
    client->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
    client->SetRemote(m_dlSockets.at(staId));
    client->SetStartTime(Time{0}); // now
    client->SetStopTime(m_duration - Simulator::Now());

    return client;
}

void
MuRtsUnprotectedReceiverTest::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);

    Config::SetDefault("ns3::WifiDefaultProtectionManager::EnableMuRts", BooleanValue(true));
    Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                       EnumValue(WifiAcknowledgment::DL_MU_TF_MU_BAR));

    NodeContainer apNode(1);
    NodeContainer staNodes(N_STAS);

    auto channel = CreateObject<MultiModelSpectrumChannel>();
    SpectrumWifiPhyHelper phy;
    phy.SetChannel(channel);
    phy.Set("ChannelSettings", StringValue("{36, 20, BAND_5GHZ, 0}"));

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ax);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("HeMcs5"),
                                 "ControlMode",
                                 StringValue("OfdmRate6Mbps"));

    WifiMacHelper mac;
    Ssid ssid("mu-rts-test");
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    auto staDevices = wifi.Install(phy, mac, staNodes);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid), "BeaconGeneration", BooleanValue(false));
    mac.SetMultiUserScheduler("ns3::RrMultiUserScheduler", "EnableUlOfdma", BooleanValue(false));
    auto apDevices = wifi.Install(phy, mac, apNode);
    auto apDev = DynamicCast<WifiNetDevice>(apDevices.Get(0));

    WifiHelper::AssignStreams(NetDeviceContainer(apDevices, staDevices), 100);

    MobilityHelper mobility;
    auto positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    for (std::size_t i = 0; i < N_STAS; ++i)
    {
        positionAlloc->Add(Vector(1.0, i * 1.0, 0.0));
    }
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(apNode);
    mobility.Install(staNodes);

    m_apMac = DynamicCast<ApWifiMac>(apDev->GetMac());
    for (std::size_t i = 0; i < N_STAS; ++i)
    {
        m_staMacs.push_back(
            DynamicCast<StaWifiMac>(DynamicCast<WifiNetDevice>(staDevices.Get(i))->GetMac()));
    }

    // skip association and Block Ack agreement establishment
    WifiStaticSetupHelper::SetStaticAssociation(apDev, staDevices);
    WifiStaticSetupHelper::SetStaticBlockAck(apDev, staDevices, {0});

    // a non-zero TXOP limit, so that both DL MU PPDUs are transmitted within one TXOP
    m_apMac->GetQosTxop(AC_BE)->SetTxopLimit(m_txopLimit);

    // trace PSDUs passed to the PHY on all devices
    for (auto devIt = NodeList::Begin(); devIt != NodeList::End(); ++devIt)
    {
        auto dev = DynamicCast<WifiNetDevice>((*devIt)->GetDevice(0));
        dev->GetPhy(0)->TraceConnectWithoutContext(
            "PhyTxPsduBegin",
            MakeCallback(&MuRtsUnprotectedReceiverTest::Transmit, this));
    }

    PacketSocketHelper packetSocket;
    packetSocket.Install(apNode);
    packetSocket.Install(staNodes);

    // install a packet socket server on each station
    for (std::size_t i = 0; i < N_STAS; ++i)
    {
        PacketSocketAddress srvAddr;
        auto staDev = DynamicCast<WifiNetDevice>(staNodes.Get(i)->GetDevice(0));
        srvAddr.SetSingleDevice(staDev->GetIfIndex());
        srvAddr.SetProtocol(1);

        auto server = CreateObject<PacketSocketServer>();
        server->SetLocal(srvAddr);
        staNodes.Get(i)->AddApplication(server);
        server->SetStartTime(Time{0});
        server->SetStopTime(m_duration);

        m_dlSockets.emplace_back();
        m_dlSockets.back().SetSingleDevice(apDev->GetIfIndex());
        m_dlSockets.back().SetPhysicalAddress(staDev->GetAddress());
        m_dlSockets.back().SetProtocol(1);
    }
}

void
MuRtsUnprotectedReceiverTest::CheckMuRts(Ptr<const WifiPsdu> psdu,
                                         const std::set<std::size_t>& staIds)
{
    CtrlTriggerHeader trigger;
    psdu->GetPayload(0)->PeekHeader(trigger);
    NS_TEST_ASSERT_MSG_EQ(trigger.IsMuRts(),
                          true,
                          "Expected an MU-RTS Trigger Frame at time "
                              << Simulator::Now().As(Time::US));

    std::set<uint16_t> expectedAids;
    for (const auto staId : staIds)
    {
        expectedAids.insert(m_staMacs.at(staId)->GetAssociationId());
    }
    std::set<uint16_t> solicitedAids;
    for (const auto& userInfo : trigger)
    {
        solicitedAids.insert(userInfo.GetAid12());
    }
    NS_TEST_EXPECT_MSG_EQ((solicitedAids == expectedAids),
                          true,
                          "The MU-RTS must solicit all receivers of the DL MU PPDU");
}

void
MuRtsUnprotectedReceiverTest::CheckDlMuPpdu(const WifiConstPsduMap& psduMap,
                                            const std::set<std::size_t>& staIds)
{
    std::set<Mac48Address> expectedReceivers;
    for (const auto staId : staIds)
    {
        expectedReceivers.insert(m_staMacs.at(staId)->GetAddress());
    }
    std::set<Mac48Address> receivers;
    for (const auto& [aid, psdu] : psduMap)
    {
        receivers.insert(psdu->GetAddr1());
    }
    NS_TEST_EXPECT_MSG_EQ((receivers == expectedReceivers),
                          true,
                          "Unexpected receiver set for the DL MU PPDU transmitted at time "
                              << Simulator::Now().As(Time::US));
}

void
MuRtsUnprotectedReceiverTest::InsertEvents()
{
    // first frame exchange: MU-RTS protecting the DL MU PPDU to STA 1 and STA 2
    m_events.emplace_back(WIFI_MAC_CTL_TRIGGER,
                          ExpectedStart::ANY,
                          [this](const WifiConstPsduMap& psduMap, const WifiTxVector& txVector) {
                              CheckMuRts(psduMap.cbegin()->second, {0, 1});
                          });
    m_events.emplace_back(WIFI_MAC_CTL_CTS, ExpectedStart::AFTER_SIFS);
    m_events.emplace_back(WIFI_MAC_CTL_CTS, ExpectedStart::SAME_TIME);
    m_events.emplace_back(WIFI_MAC_QOSDATA,
                          ExpectedStart::AFTER_SIFS,
                          [this](const WifiConstPsduMap& psduMap, const WifiTxVector& txVector) {
                              CheckDlMuPpdu(psduMap, {0, 1});
                              // generate the packets for the second DL MU PPDU, addressed to a
                              // station that responded to the MU-RTS above (STA 2) and to one that
                              // did not (STA 3)
                              for (const auto staId : {std::size_t{1}, std::size_t{2}})
                              {
                                  m_apMac->GetDevice()->GetNode()->AddApplication(
                                      GetApplication(staId));
                              }
                          });
    m_events.emplace_back(WIFI_MAC_CTL_TRIGGER,
                          ExpectedStart::AFTER_SIFS,
                          [this](const WifiConstPsduMap& psduMap, const WifiTxVector& txVector) {
                              CtrlTriggerHeader trigger;
                              psduMap.cbegin()->second->GetPayload(0)->PeekHeader(trigger);
                              NS_TEST_EXPECT_MSG_EQ(trigger.IsMuBar(),
                                                    true,
                                                    "Expected an MU-BAR Trigger Frame at time "
                                                        << Simulator::Now().As(Time::US));
                          });
    m_events.emplace_back(WIFI_MAC_CTL_BACKRESP, ExpectedStart::AFTER_SIFS);
    m_events.emplace_back(WIFI_MAC_CTL_BACKRESP, ExpectedStart::SAME_TIME);

    // second frame exchange, within the same TXOP: the DL MU PPDU to STA 2 and STA 3
    // includes an unprotected receiver (STA 3), hence another MU-RTS is required
    m_events.emplace_back(WIFI_MAC_CTL_TRIGGER,
                          ExpectedStart::AFTER_SIFS,
                          [this](const WifiConstPsduMap& psduMap, const WifiTxVector& txVector) {
                              CheckMuRts(psduMap.cbegin()->second, {1, 2});
                          });
    m_events.emplace_back(WIFI_MAC_CTL_CTS, ExpectedStart::AFTER_SIFS);
    m_events.emplace_back(WIFI_MAC_CTL_CTS, ExpectedStart::SAME_TIME);
    m_events.emplace_back(WIFI_MAC_QOSDATA,
                          ExpectedStart::AFTER_SIFS,
                          [this](const WifiConstPsduMap& psduMap, const WifiTxVector& txVector) {
                              CheckDlMuPpdu(psduMap, {1, 2});
                          });
    m_events.emplace_back(WIFI_MAC_CTL_TRIGGER,
                          ExpectedStart::AFTER_SIFS,
                          [this](const WifiConstPsduMap& psduMap, const WifiTxVector& txVector) {
                              CtrlTriggerHeader trigger;
                              psduMap.cbegin()->second->GetPayload(0)->PeekHeader(trigger);
                              NS_TEST_EXPECT_MSG_EQ(trigger.IsMuBar(),
                                                    true,
                                                    "Expected an MU-BAR Trigger Frame at time "
                                                        << Simulator::Now().As(Time::US));
                          });
    m_events.emplace_back(WIFI_MAC_CTL_BACKRESP, ExpectedStart::AFTER_SIFS);
    m_events.emplace_back(WIFI_MAC_CTL_BACKRESP, ExpectedStart::SAME_TIME);

    // all queues are empty at this point, hence the AP truncates the TXOP
    m_events.emplace_back(WIFI_MAC_CTL_END, ExpectedStart::AFTER_SIFS);
}

void
MuRtsUnprotectedReceiverTest::Transmit(WifiConstPsduMap psduMap,
                                       WifiTxVector txVector,
                                       Watt_u txPower)
{
    for (const auto& [aid, psdu] : psduMap)
    {
        std::stringstream ss;
        ss << " #MPDUs " << psdu->GetNMpdus();
        for (auto it = psdu->begin(); it != psdu->end(); ++it)
        {
            ss << "\n" << **it;
        }
        NS_LOG_INFO(ss.str());
    }
    NS_LOG_INFO("TXVECTOR = " << txVector << "\n");

    const auto psdu = psduMap.cbegin()->second;
    const auto& hdr = psdu->GetHeader(0);

    NS_TEST_EXPECT_MSG_EQ(m_events.empty(),
                          false,
                          "Unexpected frame of type " << hdr.GetTypeString()
                                                      << " transmitted at time "
                                                      << Simulator::Now().As(Time::US));
    if (m_events.empty())
    {
        return;
    }

    // check that the expected frame is being transmitted
    const auto expectedType = std::string(WifiMacHeader(m_events.front().hdrType).GetTypeString());
    NS_TEST_ASSERT_MSG_EQ(expectedType,
                          hdr.GetTypeString(),
                          "Unexpected MAC header type for frame transmitted at time "
                              << Simulator::Now().As(Time::US));

    // check the start time of the frame with respect to the previous frame
    switch (m_events.front().start)
    {
    case ExpectedStart::AFTER_SIFS:
        NS_TEST_EXPECT_MSG_EQ(Simulator::Now(),
                              m_prevTxEnd + m_apMac->GetWifiPhy(0)->GetSifs(),
                              "Frame of type " << hdr.GetTypeString()
                                               << " expected a SIFS after the previous frame");
        break;
    case ExpectedStart::SAME_TIME:
        NS_TEST_EXPECT_MSG_EQ(Simulator::Now(),
                              m_prevTxStart,
                              "Frame of type "
                                  << hdr.GetTypeString()
                                  << " expected at the same time as the previous frame");
        break;
    case ExpectedStart::ANY:
        break;
    }

    // perform actions and checks, if any (only on a frame of the expected type, as the
    // checks parse the frame contents accordingly)
    if (expectedType == hdr.GetTypeString() && m_events.front().func)
    {
        m_events.front().func(psduMap, txVector);
    }

    m_prevTxStart = Simulator::Now();
    m_prevTxEnd =
        Simulator::Now() +
        WifiPhy::CalculateTxDuration(psduMap, txVector, m_apMac->GetWifiPhy(0)->GetPhyBand());

    m_events.pop_front();
}

void
MuRtsUnprotectedReceiverTest::DoRun()
{
    InsertEvents();

    // generate the packets for the first DL MU PPDU, addressed to STA 1 and STA 2
    for (const auto staId : {std::size_t{0}, std::size_t{1}})
    {
        m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(staId));
    }

    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_events.empty(), true, "Not all events took place");

    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi protection Test Suite
 */
class WifiProtectionTestSuite : public TestSuite
{
  public:
    WifiProtectionTestSuite();
};

WifiProtectionTestSuite::WifiProtectionTestSuite()
    : TestSuite("wifi-protection", Type::UNIT)
{
    AddTestCase(new MuRtsUnprotectedReceiverTest, TestCase::Duration::QUICK);
}

static WifiProtectionTestSuite g_wifiProtectionTestSuite; ///< the test suite
