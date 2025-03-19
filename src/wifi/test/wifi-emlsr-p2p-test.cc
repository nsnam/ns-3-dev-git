/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-emlsr-p2p-test.h"

#include "ns3/advanced-ap-emlsr-manager.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/qos-frame-exchange-manager.h"
#include "ns3/qos-txop.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/string.h"
#include "ns3/wifi-net-device.h"

#include <algorithm>
#include <functional>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiEmlsrP2pTest");

EmlsrAdhocPeerTest::EmlsrAdhocPeerTest(bool switchAuxPhy, bool auxPhySleepAndUpdateCw)
    : EmlsrOperationsTestBase(
          "Check data exchange between an EMLSR client and an adhoc peer (switchAuxPhy=" +
          std::to_string(switchAuxPhy) +
          ", auxPhySleepAndUpdateCw=" + std::to_string(auxPhySleepAndUpdateCw) + ")"),
      m_switchAuxPhy(switchAuxPhy),
      m_updateCwAfterIcfFailure(auxPhySleepAndUpdateCw),
      m_staErrorModel(CreateObject<ListErrorModel>()),
      m_otherErrorModel(CreateObject<ListErrorModel>())
{
    m_linksToEnableEmlsrOn = {0, 1, 2}; // all links
    m_nEmlsrStations = 1;
    m_nNonEmlsrStations = 0;

    // channel switch delay will be also set to 64 us
    m_paddingDelay = {MicroSeconds(64)};
    m_transitionDelay = {MicroSeconds(64)};
    m_mainPhyId = 0;
    m_putAuxPhyToSleep = auxPhySleepAndUpdateCw;
    m_establishBaDl = {0};
    m_establishBaUl = {0};
    m_duration = Seconds(0.5);
}

void
EmlsrAdhocPeerTest::DoSetup()
{
    Config::SetDefault("ns3::WifiPhy::ChannelSwitchDelay", TimeValue(MicroSeconds(64)));
    Config::SetDefault("ns3::DefaultEmlsrManager::SwitchAuxPhy", BooleanValue(m_switchAuxPhy));
    Config::SetDefault("ns3::AdvancedApEmlsrManager::UpdateCwAfterFailedIcf",
                       EnumValue(m_updateCwAfterIcfFailure
                                     ? WifiUpdateCwAfterIcfFailure::ALWAYS
                                     : WifiUpdateCwAfterIcfFailure::IF_NOT_CROSS_LINK_COLLISION));
    Config::SetDefault("ns3::AdhocWifiMac::EmlsrUpdateCwAfterFailedIcf",
                       BooleanValue(m_updateCwAfterIcfFailure));

    EmlsrOperationsTestBase::DoSetup();

    m_staMacs[0]->SetAttribute("EnableP2pLinks", BooleanValue(true));

    // configure TID-to-Link Mapping so that data exchange AP<->EMLSR client occur on link 0 and 1
    auto infraLinkId = std::to_string(+m_infraLinkId);
    auto staEhtConfig = m_staMacs[0]->GetEhtConfiguration();
    staEhtConfig->SetAttribute("TidToLinkMappingDl", StringValue(std::string("0 ") + infraLinkId));
    staEhtConfig->SetAttribute("TidToLinkMappingUl", StringValue(std::string("0 ") + infraLinkId));

    // setup the adhoc peer station to operate on P2P link only
    NodeContainer adhocPeer(1);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211be);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("EhtMcs0"),
                                 "ControlMode",
                                 StringValue("HtMcs0"));

    auto p2pChannel =
        DynamicCast<MultiModelSpectrumChannel>(m_staMacs[0]->GetWifiPhy(m_p2pLinkId)->GetChannel());
    NS_TEST_ASSERT_MSG_NE(p2pChannel, nullptr, "P2P channel is not a spectrum channel");

    SpectrumWifiPhyHelper phyHelper;
    phyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phyHelper.Set("ChannelSettings", StringValue(m_channelsStr[2]));
    phyHelper.AddChannel(p2pChannel, WIFI_SPECTRUM_6_GHZ);

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac",
                "BeaconGeneration",
                BooleanValue(false), // will be enabled after enabling EMLSR mode on the client
                "EmlsrPeer",
                BooleanValue(true),
                "EmlsrPeerPaddingDelay",
                TimeValue(m_paddingDelay.front()),
                "EmlsrPeerTransitionDelay",
                TimeValue(m_transitionDelay.front()));

    auto adhocDevice = wifi.Install(phyHelper, mac, adhocPeer);
    WifiHelper::AssignStreams(adhocDevice, 100);

    auto device = DynamicCast<WifiNetDevice>(adhocDevice.Get(0));
    m_adhocMac = DynamicCast<AdhocWifiMac>(device->GetMac());

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(2.0, 0.0, 0.0));
    MobilityHelper mobility;
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(adhocPeer);

    // install packet socket on adhoc peer
    PacketSocketHelper packetSocket;
    packetSocket.Install(adhocPeer);

    // install a packet socket server on adhoc peer
    PacketSocketAddress srvAddr;
    srvAddr.SetSingleDevice(device->GetIfIndex());
    srvAddr.SetProtocol(1);

    auto server = CreateObject<PacketSocketServer>();
    server->SetLocal(srvAddr);
    adhocPeer.Get(0)->AddApplication(server);
    server->SetStartTime(Seconds(0));
    server->SetStopTime(m_duration);

    // set packet socket addresses for P2P traffic
    m_adhocToEmlsrSockAddr.SetSingleDevice(m_adhocMac->GetDevice()->GetIfIndex());
    m_adhocToEmlsrSockAddr.SetPhysicalAddress(
        m_staMacs[0]->GetFrameExchangeManager(m_p2pLinkId)->GetAddress());
    m_adhocToEmlsrSockAddr.SetProtocol(1);

    m_emlsrToAdhocSockAddr.SetSingleDevice(m_staMacs[0]->GetDevice()->GetIfIndex());
    m_emlsrToAdhocSockAddr.SetPhysicalAddress(m_adhocMac->GetDevice()->GetAddress());
    m_emlsrToAdhocSockAddr.SetProtocol(1);

    // install error models
    for (std::size_t phyId = 0; phyId < m_apMac->GetNLinks(); ++phyId)
    {
        m_apMac->GetDevice()->GetPhy(phyId)->SetPostReceptionErrorModel(m_otherErrorModel);
        m_staMacs[0]->GetDevice()->GetPhy(phyId)->SetPostReceptionErrorModel(m_staErrorModel);
    }
    m_adhocMac->GetWifiPhy(SINGLE_LINK_OP_ID)->SetPostReceptionErrorModel(m_otherErrorModel);

    // Trace PSDUs passed to the PHY on adhoc peer station
    Config::ConnectWithoutContext(
        "/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Phys/0/PhyTxPsduBegin",
        MakeCallback(&EmlsrAdhocPeerTest::Transmit, this).Bind(m_adhocMac, SINGLE_LINK_OP_ID));
}

Ptr<PacketSocketClient>
EmlsrAdhocPeerTest::GetP2pApplication(bool fromEmlsrToAdhoc,
                                      std::size_t count,
                                      std::size_t pktSize) const
{
    auto client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(pktSize));
    client->SetAttribute("MaxPackets", UintegerValue(count));
    client->SetAttribute("Interval", TimeValue(Time{0}));
    client->SetRemote(fromEmlsrToAdhoc ? m_emlsrToAdhocSockAddr : m_adhocToEmlsrSockAddr);
    client->SetStartTime(Time{0}); // now
    client->SetStopTime(m_duration - Simulator::Now());

    return client;
}

void
EmlsrAdhocPeerTest::StartTraffic()
{
    NS_LOG_FUNCTION(this);

    m_adhocMac->SetAttribute("BeaconGeneration", BooleanValue(true));

    TimeValue time;
    m_adhocMac->GetAttribute("BeaconInterval", time);

    // after 2 adhoc peer beacon intervals (to allow for Probe Request exchange), establish
    // a block ack agreement between the EMLSR client and the adhoc peer
    auto delay = 2 * time.Get();

    Simulator::Schedule(delay, [this]() {
        // stop generation of beacon frames in order to avoid possible collisions
        m_adhocMac->SetAttribute("BeaconGeneration", BooleanValue(false));

        m_staMacs[0]->GetDevice()->GetNode()->AddApplication(GetP2pApplication(true, 1, 100));
    });

    // after 5 ms (to complete establishment of block ack agreement), establish a block ack
    // agreement between the adhoc peer and the EMLSR client
    delay += MilliSeconds(5);

    Simulator::Schedule(delay, [this]() {
        m_adhocMac->GetDevice()->GetNode()->AddApplication(GetP2pApplication(false, 1, 100));
    });

    // after 5 ms (to complete establishment of block ack agreement), simulate two TXOPs
    // between the AP MLD and the EMLSR client
    delay += MilliSeconds(5);

    Simulator::Schedule(delay, [this]() {
        m_setupDone = true;
        RunOne(m_apMac);
    });

    // after 20 ms (to complete the two TXOPs with the AP MLD), simulate two TXOPs
    // between the adhoc peer and the EMLSR client
    delay += MilliSeconds(20);

    Simulator::Schedule(delay, [this]() { RunOne(m_adhocMac); });
}

void
EmlsrAdhocPeerTest::RunOne(Ptr<WifiMac> mac)
{
    NS_LOG_INFO("RUN ONE " << mac->GetTypeOfStation() << "\n");

    auto addApplication = [=, this](bool fromEmlsrClient) {
        auto sender = fromEmlsrClient ? StaticCast<WifiMac>(m_staMacs[0]) : mac;
        if (mac->GetTypeOfStation() == ADHOC_STA)
        {
            sender->GetDevice()->GetNode()->AddApplication(
                GetP2pApplication(fromEmlsrClient, 4, 1000));
        }
        else
        {
            sender->GetDevice()->GetNode()->AddApplication(
                GetApplication(fromEmlsrClient ? UPLINK : DOWNLINK, 0, 4, 1000));
        }
    };

    // generate packets addressed to the AP MLD/adhoc peer
    addApplication(true);

    // the EMLSR client has terminated a TXOP with the other device, so the main PHY has to switch
    // and an RTS is sent
    m_events.emplace_back(WIFI_MAC_CTL_RTS);

    const auto firstEventIt = std::prev(m_events.cend());

    m_events.emplace_back(
        WIFI_MAC_CTL_CTS,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkid) {
            NS_LOG_INFO("CORRUPTED\n");
            m_staErrorModel->SetList({psdu->GetPayload(0)->GetUid()});
        });

    // if SwitchAuxPhy is false, the main PHY returns on the preferred link at the CTS timeout,
    // so another RTS must be sent by the aux PHY. Otherwise, the main PHY does not switch link
    // and does not send an RTS.
    if (!m_switchAuxPhy)
    {
        m_events.emplace_back(WIFI_MAC_CTL_RTS);
        m_events.emplace_back(WIFI_MAC_CTL_CTS);
    }
    m_events.emplace_back(WIFI_MAC_QOSDATA);
    m_events.emplace_back(WIFI_MAC_CTL_BACKRESP);

    // generate packets addressed to the EMLSR client after the first TXOP
    Simulator::Schedule(MilliSeconds(10), [=]() { addApplication(false); });

    m_events.emplace_back(WIFI_MAC_CTL_TRIGGER);
    m_events.emplace_back(
        WIFI_MAC_CTL_CTS,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            NS_LOG_INFO("CORRUPTED\n");
            m_otherErrorModel->SetList({psdu->GetPayload(0)->GetUid()});
            // check that the CW value of the sender is updated appropriately after ICF failure
            const uint8_t senderLinkId = (mac == m_adhocMac ? 0 : linkId);
            // save the CW value of the sender before ICF failure
            const auto cwBefore = mac->GetQosTxop(AC_BE)->GetCw(senderLinkId);
            // check that the sender incremented CW after detecting the failure
            const auto txDuration =
                WifiPhy::CalculateTxDuration(psdu,
                                             txVector,
                                             m_staMacs[0]->GetWifiPhy(linkId)->GetPhyBand());
            Simulator::Schedule(txDuration, [=, this]() {
                const auto timeout =
                    mac->GetFrameExchangeManager(senderLinkId)->GetWifiTxTimer().GetDelayLeft();
                Simulator::Schedule(timeout, [=, this]() {
                    // the AP MLD always updates the CW because the failure is not due to a cross
                    // link collision
                    const auto cwAfter = mac->GetQosTxop(AC_BE)->GetCw(senderLinkId);
                    if (mac == m_apMac || m_updateCwAfterIcfFailure)
                    {
                        NS_TEST_EXPECT_MSG_GT(cwAfter,
                                              cwBefore,
                                              "Expected CW to be updated after ICF failure");
                    }
                    else
                    {
                        NS_TEST_EXPECT_MSG_EQ(cwAfter,
                                              cwBefore,
                                              "Expected CW not to be updated after ICF failure");
                    }
                });
            });
        });

    m_events.emplace_back(WIFI_MAC_CTL_TRIGGER);
    m_events.emplace_back(
        WIFI_MAC_CTL_CTS,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            // check that the TXOP holder at the EMLSR client is the ICF sender
            auto txopHolder =
                StaticCast<QosFrameExchangeManager>(m_staMacs[0]->GetFrameExchangeManager(linkId))
                    ->GetTxopHolder();
            NS_TEST_ASSERT_MSG_EQ(txopHolder.has_value(),
                                  true,
                                  "No TXOP holder at EMLSR client after receiving ICF");
            NS_TEST_EXPECT_MSG_EQ(m_staMacs[0]
                                      ->GetWifiRemoteStationManager(linkId)
                                      ->GetMldAddress(*txopHolder)
                                      .value_or(*txopHolder),
                                  mac->GetAddress(),
                                  "Expected the TXOP holder to be the ICF sender");
        });
    m_events.emplace_back(WIFI_MAC_QOSDATA);
    m_events.emplace_back(WIFI_MAC_CTL_BACKRESP);

    m_eventIt = firstEventIt;
}

void
EmlsrAdhocPeerTest::Transmit(Ptr<WifiMac> mac,
                             uint8_t phyId,
                             WifiConstPsduMap psduMap,
                             WifiTxVector txVector,
                             double txPowerW)
{
    EmlsrOperationsTestBase::Transmit(mac, phyId, psduMap, txVector, txPowerW);

    const auto psdu = psduMap.cbegin()->second;
    const auto& hdr = psdu->GetHeader(0);
    auto linkId = mac->GetLinkForPhy(phyId);
    NS_TEST_ASSERT_MSG_EQ(linkId.has_value(),
                          true,
                          "PHY " << +phyId << " is not operating on any link");

    // check that the EMLSR client sends frames addressed to the AP MLD on an infrastructure link
    // and frames addressed to the adhoc peer on the P2P link
    if (mac->GetTypeOfStation() == STA && m_staMacs[0]->IsEmlsrLink(0) && !hdr.GetAddr1().IsGroup())
    {
        // (unicast) PSDU transmitted by the EMLSR client after enabling EMLSR mode
        const auto toAdhocPeer = (psduMap.cbegin()->second->GetAddr1() == m_adhocMac->GetAddress());
        NS_TEST_EXPECT_MSG_EQ(toAdhocPeer,
                              (linkId.value() == m_p2pLinkId),
                              "TX occurred on unexpected link " << +linkId.value());
    }

    if (!m_setupDone || hdr.IsCfEnd())
    {
        return;
    }

    if (m_eventIt != m_events.cend())
    {
        // check that the expected frame is being transmitted
        NS_TEST_EXPECT_MSG_EQ(m_eventIt->hdrType,
                              hdr.GetType(),
                              "Unexpected MAC header type for frame #"
                                  << std::distance(m_events.cbegin(), m_eventIt));
        // perform actions/checks, if any
        if (m_eventIt->func)
        {
            m_eventIt->func(psdu, txVector, linkId.value());
        }

        ++m_eventIt;
    }
}

void
EmlsrAdhocPeerTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ((m_eventIt == m_events.cend()), true, "Not all events took place");

    Simulator::Destroy();
}

WifiEmlsrP2pTestSuite::WifiEmlsrP2pTestSuite()
    : TestSuite("wifi-emlsr-p2p", Type::UNIT)
{
    for (const auto switchAuxPhy : {true, false})
    {
        for (const auto putAuxPhyToSleep : {true, false})
        {
            AddTestCase(new EmlsrAdhocPeerTest(switchAuxPhy, putAuxPhyToSleep),
                        TestCase::Duration::QUICK);
        }
    }
}

static WifiEmlsrP2pTestSuite g_wifiEmlsrP2pTestSuite; ///< the test suite
