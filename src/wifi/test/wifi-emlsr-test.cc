/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-emlsr-test.h"

#include "ns3/attribute-container.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/ctrl-headers.h"
#include "ns3/eht-configuration.h"
#include "ns3/eht-frame-exchange-manager.h"
#include "ns3/emlsr-manager.h"
#include "ns3/log.h"
#include "ns3/mgt-action-headers.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/node-list.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/qos-txop.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/rr-multi-user-scheduler.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/string.h"
#include "ns3/wifi-net-device.h"

#include <algorithm>
#include <functional>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiEmlsrTest");

EmlOperatingModeNotificationTest::EmlOperatingModeNotificationTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of the EML Operating Mode Notification frame")
{
}

void
EmlOperatingModeNotificationTest::DoRun()
{
    MgtEmlOmn frame;

    // Both EMLSR Mode and EMLMR Mode subfields set to 0 (no link bitmap);
    TestHeaderSerialization(frame);

    frame.m_emlControl.emlsrMode = 1;
    frame.SetLinkIdInBitmap(0);
    frame.SetLinkIdInBitmap(5);
    frame.SetLinkIdInBitmap(15);

    // Adding Link Bitmap
    TestHeaderSerialization(frame);

    NS_TEST_EXPECT_MSG_EQ((frame.GetLinkBitmap() == std::list<uint8_t>{0, 5, 15}),
                          true,
                          "Unexpected link bitmap");

    auto padding = MicroSeconds(64);
    auto transition = MicroSeconds(128);

    frame.m_emlControl.emlsrParamUpdateCtrl = 1;
    frame.m_emlsrParamUpdate = MgtEmlOmn::EmlsrParamUpdate{};
    frame.m_emlsrParamUpdate->paddingDelay = CommonInfoBasicMle::EncodeEmlsrPaddingDelay(padding);
    frame.m_emlsrParamUpdate->transitionDelay =
        CommonInfoBasicMle::EncodeEmlsrTransitionDelay(transition);

    // Adding the EMLSR Parameter Update field
    TestHeaderSerialization(frame);

    NS_TEST_EXPECT_MSG_EQ(
        CommonInfoBasicMle::DecodeEmlsrPaddingDelay(frame.m_emlsrParamUpdate->paddingDelay),
        padding,
        "Unexpected EMLSR Padding Delay");
    NS_TEST_EXPECT_MSG_EQ(
        CommonInfoBasicMle::DecodeEmlsrTransitionDelay(frame.m_emlsrParamUpdate->transitionDelay),
        transition,
        "Unexpected EMLSR Transition Delay");
}

EmlsrOperationsTestBase::EmlsrOperationsTestBase(const std::string& name)
    : TestCase(name)
{
}

void
EmlsrOperationsTestBase::Transmit(Ptr<WifiMac> mac,
                                  uint8_t phyId,
                                  WifiConstPsduMap psduMap,
                                  WifiTxVector txVector,
                                  double txPowerW)
{
    auto linkId = mac->GetLinkForPhy(phyId);
    NS_TEST_ASSERT_MSG_EQ(linkId.has_value(), true, "No link found for PHY ID " << +phyId);
    m_txPsdus.push_back({Simulator::Now(), psduMap, txVector, *linkId, phyId});

    auto txDuration =
        WifiPhy::CalculateTxDuration(psduMap, txVector, mac->GetWifiPhy(*linkId)->GetPhyBand());

    for (const auto& [aid, psdu] : psduMap)
    {
        std::stringstream ss;
        ss << std::setprecision(10) << "PSDU #" << m_txPsdus.size() << " Link ID "
           << +linkId.value() << " Phy ID " << +phyId << " " << psdu->GetHeader(0).GetTypeString();
        if (psdu->GetHeader(0).IsAction())
        {
            ss << " ";
            WifiActionHeader actionHdr;
            psdu->GetPayload(0)->PeekHeader(actionHdr);
            actionHdr.Print(ss);
        }
        ss << " #MPDUs " << psdu->GetNMpdus() << " duration/ID " << psdu->GetHeader(0).GetDuration()
           << " RA = " << psdu->GetAddr1() << " TA = " << psdu->GetAddr2()
           << " ADDR3 = " << psdu->GetHeader(0).GetAddr3()
           << " ToDS = " << psdu->GetHeader(0).IsToDs()
           << " FromDS = " << psdu->GetHeader(0).IsFromDs();
        if (psdu->GetHeader(0).IsQosData())
        {
            ss << " seqNo = {";
            for (auto& mpdu : *PeekPointer(psdu))
            {
                ss << mpdu->GetHeader().GetSequenceNumber() << ",";
            }
            ss << "} TID = " << +psdu->GetHeader(0).GetQosTid();
        }
        NS_LOG_INFO(ss.str());

        // if this frame is transmitted by an EMLSR client on an EMLSR links, in-device interference
        // is configured and the TX duration exceeds the threshold (72us), MediumSyncDelay timer is
        // (re)started at the end of the transmission
        if (auto staMac = DynamicCast<StaWifiMac>(mac);
            staMac && staMac->IsEmlsrLink(*linkId) &&
            staMac->GetEmlsrManager()->GetMediumSyncDuration().IsStrictlyPositive())
        {
            const auto mustStartMsd =
                staMac->GetEmlsrManager()->GetInDeviceInterference() &&
                txDuration > MicroSeconds(EmlsrManager::MEDIUM_SYNC_THRESHOLD_USEC);

            for (auto id : staMac->GetLinkIds())
            {
                // timer started on EMLSR links other than the link on which TX is starting,
                // provided that a PHY is operating on the link and MediumSyncDuration is not null
                if (!staMac->IsEmlsrLink(id) || id == *linkId || staMac->GetWifiPhy(id) == nullptr)
                {
                    continue;
                }
                Simulator::Schedule(
                    txDuration - TimeStep(1),
                    [=, hdrType = psdu->GetHeader(0).GetTypeString(), this]() {
                        // check if MSD timer was running on the link before completing transmission
                        bool msdWasRunning = staMac->GetEmlsrManager()
                                                 ->GetElapsedMediumSyncDelayTimer(id)
                                                 .has_value();
                        if (auto phy = staMac->GetWifiPhy(id);
                            !msdWasRunning && !mustStartMsd && phy && phy->IsStateSleep())
                        {
                            // if the MSD timer was not running before the end of the TX, it is not
                            // expected to be started and the PHY operating on this link is
                            // sleeping, do not check that the MSD timer is not started after the
                            // end of the TX, because it may be started because of the sleep period
                            // of the aux PHY
                            return;
                        }
                        Simulator::Schedule(TimeStep(2), [=, this]() {
                            CheckMsdTimerRunning(staMac,
                                                 id,
                                                 (msdWasRunning || mustStartMsd),
                                                 std::string("after transmitting ") + hdrType +
                                                     " on link " + std::to_string(*linkId));
                        });
                    });
            }
        }
    }
    NS_LOG_INFO("TX duration = " << txDuration.As(Time::MS) << "  TXVECTOR = " << txVector << "\n");
}

void
EmlsrOperationsTestBase::CheckMsdTimerRunning(Ptr<StaWifiMac> staMac,
                                              uint8_t linkId,
                                              bool isRunning,
                                              const std::string& msg)
{
    auto time = staMac->GetEmlsrManager()->GetElapsedMediumSyncDelayTimer(linkId);
    NS_TEST_ASSERT_MSG_EQ(time.has_value(),
                          isRunning,
                          Simulator::Now().As(Time::MS)
                              << " Unexpected status for MediumSyncDelay timer on link " << +linkId
                              << " " << msg);
    if (auto phy = staMac->GetWifiPhy(linkId))
    {
        auto currThreshold = phy->GetCcaEdThreshold();
        NS_TEST_EXPECT_MSG_EQ((static_cast<int8_t>(currThreshold) ==
                               staMac->GetEmlsrManager()->GetMediumSyncOfdmEdThreshold()),
                              isRunning,
                              Simulator::Now().As(Time::MS)
                                  << " Unexpected value (" << currThreshold
                                  << ") for CCA ED threshold on link " << +linkId << " " << msg);
    }
}

void
EmlsrOperationsTestBase::CheckAuxPhysSleepMode(Ptr<StaWifiMac> staMac, bool sleep)
{
    if (!m_putAuxPhyToSleep)
    {
        // if m_putAuxPhyToSleep is false, aux PHYs must not be put to sleep
        sleep = false;
    }

    for (const auto& phy : staMac->GetDevice()->GetPhys())
    {
        if (phy->GetPhyId() == m_mainPhyId)
        {
            continue; // do not check the main PHY
        }

        auto linkId = staMac->GetLinkForPhy(phy);

        if (linkId.has_value() && !staMac->IsEmlsrLink(*linkId))
        {
            continue; // this PHY is not operating on an EMLSR link
        }

        if (!sleep)
        {
            NS_TEST_EXPECT_MSG_EQ(phy->IsStateSleep(),
                                  false,
                                  Simulator::Now().GetTimeStep()
                                      << " PHY " << +phy->GetPhyId() << " is in unexpected state "
                                      << phy->GetState()->GetState());
            continue;
        }

        // if the PHY is in state TX or switching, sleep is postponed until their end
        const auto delay =
            (phy->IsStateTx() || phy->IsStateSwitching()) ? phy->GetDelayUntilIdle() : Time{0};

        Simulator::Schedule(delay, [=, this]() {
            NS_TEST_EXPECT_MSG_EQ(phy->IsStateSleep(),
                                  true,
                                  "PHY " << +phy->GetPhyId() << " is in unexpected state "
                                         << phy->GetState()->GetState());
        });
    }
}

void
EmlsrOperationsTestBase::MainPhySwitchInfoCallback(std::size_t index,
                                                   const EmlsrMainPhySwitchTrace& info)
{
    m_traceInfo[index] = info.Clone();
}

void
EmlsrOperationsTestBase::CheckMainPhyTraceInfo(std::size_t index,
                                               std::string_view reason,
                                               const std::optional<uint8_t>& fromLinkId,
                                               uint8_t toLinkId,
                                               bool checkFromLinkId,
                                               bool checkToLinkId)
{
    const auto traceInfoIt = m_traceInfo.find(index);
    NS_TEST_ASSERT_MSG_EQ((traceInfoIt != m_traceInfo.cend()), true, "Expected stored trace info");
    const auto& traceInfo = traceInfoIt->second;

    NS_TEST_EXPECT_MSG_EQ(traceInfo->GetName(), reason, "Unexpected reason");

    if (checkFromLinkId)
    {
        NS_TEST_ASSERT_MSG_EQ(traceInfo->fromLinkId.has_value(),
                              fromLinkId.has_value(),
                              "Unexpected stored from_link ID");
        if (fromLinkId.has_value())
        {
            NS_TEST_EXPECT_MSG_EQ(+traceInfo->fromLinkId.value(),
                                  +fromLinkId.value(),
                                  "Unexpected from_link ID");
        }
    }

    if (checkToLinkId)
    {
        NS_TEST_EXPECT_MSG_EQ(+traceInfo->toLinkId, +toLinkId, "Unexpected to_link ID");
    }

    m_traceInfo.erase(traceInfoIt);
}

void
EmlsrOperationsTestBase::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 100;

    Config::SetDefault("ns3::WifiMac::MpduBufferSize", UintegerValue(64));
    Config::SetDefault("ns3::EmlsrManager::InDeviceInterference", BooleanValue(true));
    Config::SetDefault("ns3::EmlsrManager::PutAuxPhyToSleep", BooleanValue(m_putAuxPhyToSleep));

    NodeContainer wifiApNode(1);
    NodeContainer wifiStaNodes(m_nEmlsrStations);

    WifiHelper wifi;
    // wifi.EnableLogComponents ();
    wifi.SetStandard(WIFI_STANDARD_80211be);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("EhtMcs0"),
                                 "ControlMode",
                                 StringValue("HtMcs0"));
    wifi.ConfigEhtOptions("EmlsrActivated",
                          BooleanValue(true),
                          "TransitionTimeout",
                          TimeValue(m_transitionTimeout));

    // MLDs are configured with three links
    SpectrumWifiPhyHelper phyHelper(3);
    phyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phyHelper.SetPcapCaptureType(WifiPhyHelper::PcapCaptureType::PCAP_PER_LINK);
    phyHelper.Set(0, "ChannelSettings", StringValue("{2, 0, BAND_2_4GHZ, 0}"));
    phyHelper.Set(1, "ChannelSettings", StringValue("{36, 0, BAND_5GHZ, 0}"));
    phyHelper.Set(2, "ChannelSettings", StringValue("{1, 0, BAND_6GHZ, 0}"));
    // Add three spectrum channels to use multi-RF interface
    phyHelper.AddChannel(CreateObject<MultiModelSpectrumChannel>(), WIFI_SPECTRUM_2_4_GHZ);
    phyHelper.AddChannel(CreateObject<MultiModelSpectrumChannel>(), WIFI_SPECTRUM_5_GHZ);
    phyHelper.AddChannel(CreateObject<MultiModelSpectrumChannel>(), WIFI_SPECTRUM_6_GHZ);

    WifiMacHelper mac;
    mac.SetType("ns3::ApWifiMac",
                "Ssid",
                SsidValue(Ssid("ns-3-ssid")),
                "BeaconGeneration",
                BooleanValue(true));
    mac.SetApEmlsrManager("ns3::AdvancedApEmlsrManager",
                          "WaitTransDelayOnPsduRxError",
                          BooleanValue(true));

    NetDeviceContainer apDevice = wifi.Install(phyHelper, mac, wifiApNode);

    mac.SetType("ns3::StaWifiMac",
                "Ssid",
                SsidValue(Ssid("wrong-ssid")),
                "MaxMissedBeacons",
                UintegerValue(1e6), // do not deassociate
                "ActiveProbing",
                BooleanValue(false));
    mac.SetEmlsrManager("ns3::AdvancedEmlsrManager",
                        "EmlsrLinkSet",
                        AttributeContainerValue<UintegerValue>(m_linksToEnableEmlsrOn),
                        "MainPhyId",
                        UintegerValue(m_mainPhyId));

    NetDeviceContainer staDevices = wifi.Install(phyHelper, mac, wifiStaNodes);

    m_apMac = DynamicCast<ApWifiMac>(DynamicCast<WifiNetDevice>(apDevice.Get(0))->GetMac());

    for (uint32_t i = 0; i < staDevices.GetN(); i++)
    {
        auto device = DynamicCast<WifiNetDevice>(staDevices.Get(i));
        auto staMac = DynamicCast<StaWifiMac>(device->GetMac());
        auto emlsrManager = staMac->GetEmlsrManager();
        NS_ASSERT_MSG(i < m_paddingDelay.size(), "Not enough padding delay values provided");
        emlsrManager->SetAttribute("EmlsrPaddingDelay", TimeValue(m_paddingDelay.at(i)));
        NS_ASSERT_MSG(i < m_transitionDelay.size(), "Not enough transition delay values provided");
        emlsrManager->SetAttribute("EmlsrTransitionDelay", TimeValue(m_transitionDelay.at(i)));
        emlsrManager->TraceConnectWithoutContext(
            "MainPhySwitch",
            MakeCallback(&EmlsrOperationsTestBase::MainPhySwitchInfoCallback, this).Bind(i));
    }

    if (m_nNonEmlsrStations > 0)
    {
        // create the other non-AP MLDs for which EMLSR is not activated
        wifi.ConfigEhtOptions("EmlsrActivated", BooleanValue(false));
        NodeContainer otherStaNodes(m_nNonEmlsrStations);
        staDevices.Add(wifi.Install(phyHelper, mac, otherStaNodes));
        wifiStaNodes.Add(otherStaNodes);
    }

    for (uint32_t i = 0; i < staDevices.GetN(); i++)
    {
        auto device = DynamicCast<WifiNetDevice>(staDevices.Get(i));
        m_staMacs.push_back(DynamicCast<StaWifiMac>(device->GetMac()));
    }

    // Trace PSDUs passed to the PHY on AP MLD and non-AP MLDs
    for (uint8_t phyId = 0; phyId < m_apMac->GetDevice()->GetNPhys(); phyId++)
    {
        Config::ConnectWithoutContext(
            "/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Phys/" + std::to_string(phyId) +
                "/PhyTxPsduBegin",
            MakeCallback(&EmlsrOperationsTestBase::Transmit, this).Bind(m_apMac, phyId));
    }
    for (std::size_t i = 0; i < m_nEmlsrStations + m_nNonEmlsrStations; i++)
    {
        for (uint8_t phyId = 0; phyId < m_staMacs[i]->GetDevice()->GetNPhys(); phyId++)
        {
            Config::ConnectWithoutContext(
                "/NodeList/" + std::to_string(i + 1) + "/DeviceList/*/$ns3::WifiNetDevice/Phys/" +
                    std::to_string(phyId) + "/PhyTxPsduBegin",
                MakeCallback(&EmlsrOperationsTestBase::Transmit, this).Bind(m_staMacs[i], phyId));
        }
    }

    // Uncomment the lines below to write PCAP files
    // phyHelper.EnablePcap("wifi-emlsr_AP", apDevice);
    // phyHelper.EnablePcap("wifi-emlsr_STA", staDevices);

    // Assign fixed streams to random variables in use
    streamNumber += WifiHelper::AssignStreams(apDevice, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevices, streamNumber);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    for (std::size_t id = 0; id <= m_nEmlsrStations + m_nNonEmlsrStations; id++)
    {
        // all non-AP MLDs are co-located
        positionAlloc->Add(Vector(std::min<double>(id, 1), 0.0, 0.0));
    }
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    // install packet socket on all nodes
    PacketSocketHelper packetSocket;
    packetSocket.Install(wifiApNode);
    packetSocket.Install(wifiStaNodes);

    // install a packet socket server on all nodes
    for (auto nodeIt = NodeList::Begin(); nodeIt != NodeList::End(); nodeIt++)
    {
        PacketSocketAddress srvAddr;
        auto device = DynamicCast<WifiNetDevice>((*nodeIt)->GetDevice(0));
        NS_TEST_ASSERT_MSG_NE(device, nullptr, "Expected a WifiNetDevice");
        srvAddr.SetSingleDevice(device->GetIfIndex());
        srvAddr.SetProtocol(1);

        auto server = CreateObject<PacketSocketServer>();
        server->SetLocal(srvAddr);
        (*nodeIt)->AddApplication(server);
        server->SetStartTime(Seconds(0)); // now
        server->SetStopTime(m_duration);
    }

    // set DL and UL packet sockets
    for (const auto& staMac : m_staMacs)
    {
        m_dlSockets.emplace_back();
        m_dlSockets.back().SetSingleDevice(m_apMac->GetDevice()->GetIfIndex());
        m_dlSockets.back().SetPhysicalAddress(staMac->GetDevice()->GetAddress());
        m_dlSockets.back().SetProtocol(1);

        m_ulSockets.emplace_back();
        m_ulSockets.back().SetSingleDevice(staMac->GetDevice()->GetIfIndex());
        m_ulSockets.back().SetPhysicalAddress(m_apMac->GetDevice()->GetAddress());
        m_ulSockets.back().SetProtocol(1);
    }

    // schedule ML setup for one station at a time
    m_apMac->TraceConnectWithoutContext("AssociatedSta",
                                        MakeCallback(&EmlsrOperationsTestBase::SetSsid, this));
    Simulator::Schedule(Seconds(0), [&]() { m_staMacs[0]->SetSsid(Ssid("ns-3-ssid")); });
}

Ptr<PacketSocketClient>
EmlsrOperationsTestBase::GetApplication(TrafficDirection dir,
                                        std::size_t staId,
                                        std::size_t count,
                                        std::size_t pktSize) const
{
    auto client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(pktSize));
    client->SetAttribute("MaxPackets", UintegerValue(count));
    client->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
    client->SetRemote(dir == DOWNLINK ? m_dlSockets.at(staId) : m_ulSockets.at(staId));
    client->SetStartTime(Seconds(0)); // now
    client->SetStopTime(m_duration - Simulator::Now());

    return client;
}

void
EmlsrOperationsTestBase::SetSsid(uint16_t aid, Mac48Address /* addr */)
{
    if (m_lastAid == aid)
    {
        // another STA of this non-AP MLD has already fired this callback
        return;
    }
    m_lastAid = aid;

    // wait some time (5ms) to allow the completion of association
    auto delay = MilliSeconds(5);

    if (m_establishBaDl)
    {
        // trigger establishment of BA agreement with AP as originator
        Simulator::Schedule(delay, [=, this]() {
            m_apMac->GetDevice()->GetNode()->AddApplication(
                GetApplication(DOWNLINK, aid - 1, 4, 1000));
        });

        delay += MilliSeconds(5);
    }

    if (m_establishBaUl)
    {
        // trigger establishment of BA agreement with AP as recipient
        Simulator::Schedule(delay, [=, this]() {
            m_staMacs[aid - 1]->GetDevice()->GetNode()->AddApplication(
                GetApplication(UPLINK, aid - 1, 4, 1000));
        });

        delay += MilliSeconds(5);
    }

    Simulator::Schedule(delay, [=, this]() {
        if (aid < m_nEmlsrStations + m_nNonEmlsrStations)
        {
            // make the next STA start ML discovery & setup
            m_staMacs[aid]->SetSsid(Ssid("ns-3-ssid"));
            return;
        }
        // all stations associated; start traffic if needed
        StartTraffic();
        // stop generation of beacon frames in order to avoid interference
        m_apMac->SetAttribute("BeaconGeneration", BooleanValue(false));
    });
}

void
EmlsrOperationsTestBase::CheckBlockedLink(Ptr<WifiMac> mac,
                                          Mac48Address dest,
                                          uint8_t linkId,
                                          WifiQueueBlockedReason reason,
                                          bool blocked,
                                          std::string description,
                                          bool testUnblockedForOtherReasons)
{
    WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, dest, 0);
    auto mask = mac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, linkId);
    NS_TEST_EXPECT_MSG_EQ(mask.has_value(),
                          true,
                          description << ": Expected to find a mask for EMLSR link " << +linkId);
    if (blocked)
    {
        NS_TEST_EXPECT_MSG_EQ(mask->test(static_cast<std::size_t>(reason)),
                              true,
                              description << ": Expected EMLSR link " << +linkId
                                          << " to be blocked for reason " << reason);
        if (testUnblockedForOtherReasons)
        {
            NS_TEST_EXPECT_MSG_EQ(mask->count(),
                                  1,
                                  description << ": Expected EMLSR link " << +linkId
                                              << " to be blocked for one reason only");
        }
    }
    else if (testUnblockedForOtherReasons)
    {
        NS_TEST_EXPECT_MSG_EQ(mask->none(),
                              true,
                              description << ": Expected EMLSR link " << +linkId
                                          << " to be unblocked");
    }
    else
    {
        NS_TEST_EXPECT_MSG_EQ(mask->test(static_cast<std::size_t>(reason)),
                              false,
                              description << ": Expected EMLSR link " << +linkId
                                          << " to be unblocked for reason " << reason);
    }
}

EmlOmnExchangeTest::EmlOmnExchangeTest(const std::set<uint8_t>& linksToEnableEmlsrOn,
                                       Time transitionTimeout)
    : EmlsrOperationsTestBase("Check EML Notification exchange"),
      m_checkEmlsrLinksCount(0),
      m_emlNotificationDroppedCount(0)
{
    m_linksToEnableEmlsrOn = linksToEnableEmlsrOn;
    m_nEmlsrStations = 1;
    m_nNonEmlsrStations = 0;
    m_transitionTimeout = transitionTimeout;
    m_duration = Seconds(0.5);
}

void
EmlOmnExchangeTest::DoSetup()
{
    EmlsrOperationsTestBase::DoSetup();

    m_errorModel = CreateObject<ListErrorModel>();
    for (std::size_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
    {
        m_apMac->GetWifiPhy(linkId)->SetPostReceptionErrorModel(m_errorModel);
    }

    m_staMacs[0]->TraceConnectWithoutContext("AckedMpdu",
                                             MakeCallback(&EmlOmnExchangeTest::TxOk, this));
    m_staMacs[0]->TraceConnectWithoutContext("DroppedMpdu",
                                             MakeCallback(&EmlOmnExchangeTest::TxDropped, this));
}

void
EmlOmnExchangeTest::Transmit(Ptr<WifiMac> mac,
                             uint8_t phyId,
                             WifiConstPsduMap psduMap,
                             WifiTxVector txVector,
                             double txPowerW)
{
    EmlsrOperationsTestBase::Transmit(mac, phyId, psduMap, txVector, txPowerW);
    auto linkId = m_txPsdus.back().linkId;

    auto psdu = psduMap.begin()->second;

    switch (psdu->GetHeader(0).GetType())
    {
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
        NS_TEST_EXPECT_MSG_EQ(+linkId, +m_mainPhyId, "AssocReq not sent by the main PHY");
        CheckEmlCapabilitiesInAssocReq(*psdu->begin(), txVector, linkId);
        break;

    case WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
        CheckEmlCapabilitiesInAssocResp(*psdu->begin(), txVector, linkId);
        break;

    case WIFI_MAC_MGT_ACTION:
        if (auto [category, action] = WifiActionHeader::Peek(psdu->GetPayload(0));
            category == WifiActionHeader::PROTECTED_EHT &&
            action.protectedEhtAction ==
                WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION)
        {
            CheckEmlNotification(psdu, txVector, linkId);

            if (m_emlNotificationDroppedCount == 0 &&
                m_staMacs[0]->GetLinkIdByAddress(psdu->GetAddr2()) == linkId)
            {
                // transmitted by non-AP MLD, we need to corrupt it
                m_uidList.push_front(psdu->GetPacket()->GetUid());
                m_errorModel->SetList(m_uidList);
            }
            break;
        }

    default:;
    }
}

void
EmlOmnExchangeTest::CheckEmlCapabilitiesInAssocReq(Ptr<const WifiMpdu> mpdu,
                                                   const WifiTxVector& txVector,
                                                   uint8_t linkId)
{
    MgtAssocRequestHeader frame;
    mpdu->GetPacket()->PeekHeader(frame);

    const auto& mle = frame.Get<MultiLinkElement>();
    NS_TEST_ASSERT_MSG_EQ(mle.has_value(), true, "Multi-Link Element must be present in AssocReq");

    NS_TEST_ASSERT_MSG_EQ(mle->HasEmlCapabilities(),
                          true,
                          "Multi-Link Element in AssocReq must have EML Capabilities");
    NS_TEST_ASSERT_MSG_EQ(mle->IsEmlsrSupported(),
                          true,
                          "EML Support subfield of EML Capabilities in AssocReq must be set to 1");
    NS_TEST_ASSERT_MSG_EQ(mle->GetEmlsrPaddingDelay(),
                          m_paddingDelay.at(0),
                          "Unexpected Padding Delay in EML Capabilities included in AssocReq");
    NS_TEST_ASSERT_MSG_EQ(mle->GetEmlsrTransitionDelay(),
                          m_transitionDelay.at(0),
                          "Unexpected Transition Delay in EML Capabilities included in AssocReq");
}

void
EmlOmnExchangeTest::CheckEmlCapabilitiesInAssocResp(Ptr<const WifiMpdu> mpdu,
                                                    const WifiTxVector& txVector,
                                                    uint8_t linkId)
{
    bool sentToEmlsrClient =
        (m_staMacs[0]->GetLinkIdByAddress(mpdu->GetHeader().GetAddr1()) == linkId);

    if (!sentToEmlsrClient)
    {
        // nothing to check
        return;
    }

    MgtAssocResponseHeader frame;
    mpdu->GetPacket()->PeekHeader(frame);

    const auto& mle = frame.Get<MultiLinkElement>();
    NS_TEST_ASSERT_MSG_EQ(mle.has_value(), true, "Multi-Link Element must be present in AssocResp");

    NS_TEST_ASSERT_MSG_EQ(mle->HasEmlCapabilities(),
                          true,
                          "Multi-Link Element in AssocResp must have EML Capabilities");
    NS_TEST_ASSERT_MSG_EQ(mle->IsEmlsrSupported(),
                          true,
                          "EML Support subfield of EML Capabilities in AssocResp must be set to 1");
    NS_TEST_ASSERT_MSG_EQ(
        mle->GetTransitionTimeout(),
        m_transitionTimeout,
        "Unexpected Transition Timeout in EML Capabilities included in AssocResp");
}

void
EmlOmnExchangeTest::CheckEmlNotification(Ptr<const WifiPsdu> psdu,
                                         const WifiTxVector& txVector,
                                         uint8_t linkId)
{
    MgtEmlOmn frame;
    auto mpdu = *psdu->begin();
    auto pkt = mpdu->GetPacket()->Copy();
    WifiActionHeader::Remove(pkt);
    pkt->RemoveHeader(frame);
    NS_LOG_DEBUG(frame);

    bool sentbyNonApMld = m_staMacs[0]->GetLinkIdByAddress(mpdu->GetHeader().GetAddr2()) == linkId;

    NS_TEST_EXPECT_MSG_EQ(+frame.m_emlControl.emlsrMode,
                          1,
                          "EMLSR Mode subfield should be set to 1 (frame sent by non-AP MLD: "
                              << std::boolalpha << sentbyNonApMld << ")");

    NS_TEST_EXPECT_MSG_EQ(+frame.m_emlControl.emlmrMode,
                          0,
                          "EMLMR Mode subfield should be set to 0 (frame sent by non-AP MLD: "
                              << std::boolalpha << sentbyNonApMld << ")");

    NS_TEST_ASSERT_MSG_EQ(frame.m_emlControl.linkBitmap.has_value(),
                          true,
                          "Link Bitmap subfield should be present (frame sent by non-AP MLD: "
                              << std::boolalpha << sentbyNonApMld << ")");

    auto setupLinks = m_staMacs[0]->GetSetupLinkIds();
    std::list<uint8_t> expectedEmlsrLinks;
    std::set_intersection(setupLinks.begin(),
                          setupLinks.end(),
                          m_linksToEnableEmlsrOn.begin(),
                          m_linksToEnableEmlsrOn.end(),
                          std::back_inserter(expectedEmlsrLinks));

    NS_TEST_EXPECT_MSG_EQ((expectedEmlsrLinks == frame.GetLinkBitmap()),
                          true,
                          "Unexpected Link Bitmap subfield (frame sent by non-AP MLD: "
                              << std::boolalpha << sentbyNonApMld << ")");

    if (!sentbyNonApMld)
    {
        // the frame has been sent by the AP MLD
        NS_TEST_ASSERT_MSG_EQ(
            +frame.m_emlControl.emlsrParamUpdateCtrl,
            0,
            "EMLSR Parameter Update Control should be set to 0 in frames sent by the AP MLD");

        // as soon as the non-AP MLD receives this frame, it sets the EMLSR links
        auto delay = WifiPhy::CalculateTxDuration(psdu,
                                                  txVector,
                                                  m_staMacs[0]->GetWifiPhy(linkId)->GetPhyBand()) +
                     MicroSeconds(1); // to account for propagation delay
        Simulator::Schedule(delay, &EmlOmnExchangeTest::CheckEmlsrLinks, this);
    }

    NS_TEST_EXPECT_MSG_EQ(+m_mainPhyId,
                          +linkId,
                          "EML Notification received on unexpected link (frame sent by non-AP MLD: "
                              << std::boolalpha << sentbyNonApMld << ")");
}

void
EmlOmnExchangeTest::TxOk(Ptr<const WifiMpdu> mpdu)
{
    const auto& hdr = mpdu->GetHeader();

    if (hdr.IsMgt() && hdr.IsAction())
    {
        if (auto [category, action] = WifiActionHeader::Peek(mpdu->GetPacket());
            category == WifiActionHeader::PROTECTED_EHT &&
            action.protectedEhtAction ==
                WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION)
        {
            // the EML Operating Mode Notification frame that the non-AP MLD sent has been
            // acknowledged; after the transition timeout, the EMLSR links have been set
            Simulator::Schedule(m_transitionTimeout + NanoSeconds(1),
                                &EmlOmnExchangeTest::CheckEmlsrLinks,
                                this);
        }
    }
}

void
EmlOmnExchangeTest::TxDropped(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu)
{
    const auto& hdr = mpdu->GetHeader();

    if (hdr.IsMgt() && hdr.IsAction())
    {
        if (auto [category, action] = WifiActionHeader::Peek(mpdu->GetPacket());
            category == WifiActionHeader::PROTECTED_EHT &&
            action.protectedEhtAction ==
                WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION)
        {
            // the EML Operating Mode Notification frame has been dropped. Don't
            // corrupt it anymore
            m_emlNotificationDroppedCount++;
        }
    }
}

void
EmlOmnExchangeTest::CheckEmlsrLinks()
{
    m_checkEmlsrLinksCount++;

    auto setupLinks = m_staMacs[0]->GetSetupLinkIds();
    std::set<uint8_t> expectedEmlsrLinks;
    std::set_intersection(setupLinks.begin(),
                          setupLinks.end(),
                          m_linksToEnableEmlsrOn.begin(),
                          m_linksToEnableEmlsrOn.end(),
                          std::inserter(expectedEmlsrLinks, expectedEmlsrLinks.end()));

    NS_TEST_EXPECT_MSG_EQ((expectedEmlsrLinks == m_staMacs[0]->GetEmlsrManager()->GetEmlsrLinks()),
                          true,
                          "Unexpected set of EMLSR links)");
}

void
EmlOmnExchangeTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_checkEmlsrLinksCount,
                          2,
                          "Unexpected number of times CheckEmlsrLinks() is called");
    NS_TEST_EXPECT_MSG_EQ(
        m_emlNotificationDroppedCount,
        1,
        "Unexpected number of times the EML Notification frame is dropped due to max retry limit");

    Simulator::Destroy();
}

EmlsrDlTxopTest::EmlsrDlTxopTest(const Params& params)
    : EmlsrOperationsTestBase("Check EML DL TXOP transmissions (" +
                              std::to_string(params.nEmlsrStations) + "," +
                              std::to_string(params.nNonEmlsrStations) + ")"),
      m_emlsrLinks(params.linksToEnableEmlsrOn),
      m_emlsrEnabledTime(0),
      m_fe2to3delay(MilliSeconds(20)),
      m_countQoSframes(0),
      m_countBlockAck(0)
{
    m_nEmlsrStations = params.nEmlsrStations;
    m_nNonEmlsrStations = params.nNonEmlsrStations;
    m_linksToEnableEmlsrOn = {}; // do not enable EMLSR right after association
    m_mainPhyId = 1;
    m_paddingDelay = params.paddingDelay;
    m_transitionDelay = params.transitionDelay;
    m_transitionTimeout = params.transitionTimeout;
    m_establishBaDl = true;
    m_putAuxPhyToSleep = params.putAuxPhyToSleep;
    m_duration = Seconds(1.5);

    NS_ABORT_MSG_IF(params.linksToEnableEmlsrOn.size() < 2,
                    "This test requires at least two links to be configured as EMLSR links");
}

void
EmlsrDlTxopTest::Transmit(Ptr<WifiMac> mac,
                          uint8_t phyId,
                          WifiConstPsduMap psduMap,
                          WifiTxVector txVector,
                          double txPowerW)
{
    EmlsrOperationsTestBase::Transmit(mac, phyId, psduMap, txVector, txPowerW);
    auto linkId = m_txPsdus.back().linkId;

    auto psdu = psduMap.begin()->second;
    auto nodeId = mac->GetDevice()->GetNode()->GetId();

    switch (psdu->GetHeader(0).GetType())
    {
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
        NS_ASSERT_MSG(nodeId > 0, "APs do not send AssocReq frames");
        if (nodeId <= m_nEmlsrStations)
        {
            NS_TEST_EXPECT_MSG_EQ(+linkId, +m_mainPhyId, "AssocReq not sent by the main PHY");
            // this AssocReq is being sent by an EMLSR client. The other EMLSR links should be
            // in powersave mode after association; we let the non-EMLSR links transition to
            // active mode (by sending data null frames) after association
            for (const auto id : m_staMacs.at(nodeId - 1)->GetLinkIds())
            {
                if (id != linkId && m_emlsrLinks.count(id) == 1)
                {
                    m_staMacs[nodeId - 1]->SetPowerSaveMode({true, id});
                }
            }
        }
        break;

    case WIFI_MAC_MGT_ACTION: {
        auto [category, action] = WifiActionHeader::Peek(psdu->GetPayload(0));

        if ((category == WifiActionHeader::PROTECTED_EHT) &&
            (action.protectedEhtAction ==
             WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION))
        {
            nodeId == 0 ? CheckApEmlNotificationFrame(*psdu->begin(), txVector, linkId)
                        : CheckStaEmlNotificationFrame(*psdu->begin(), txVector, linkId);
        }
        else if (category == WifiActionHeader::BLOCK_ACK &&
                 action.blockAck == WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST)
        {
            CheckPmModeAfterAssociation(psdu->GetAddr1());
        }
    }
    break;

    case WIFI_MAC_CTL_TRIGGER:
        CheckInitialControlFrame(*psdu->begin(), txVector, linkId);
        break;

    case WIFI_MAC_QOSDATA:
        CheckQosFrames(psduMap, txVector, linkId);
        break;

    case WIFI_MAC_CTL_BACKRESP:
        CheckBlockAck(psduMap, txVector, phyId);
        break;

    case WIFI_MAC_CTL_END:
        if (auto apMac = DynamicCast<ApWifiMac>(mac))
        {
            const auto txDuration =
                WifiPhy::CalculateTxDuration(psduMap,
                                             txVector,
                                             apMac->GetDevice()->GetPhy(phyId)->GetPhyBand());
            for (std::size_t i = 0; i < m_nEmlsrStations; ++i)
            {
                if (m_staMacs[i]->IsEmlsrLink(linkId) &&
                    m_staMacs[i]->GetWifiPhy(linkId) ==
                        m_staMacs[i]->GetDevice()->GetPhy(m_mainPhyId))
                {
                    // AP is terminating a TXOP on an EMLSR link on which the main PHY is operating,
                    // aux PHYs should resume from sleep
                    Simulator::Schedule(txDuration + TimeStep(1),
                                        &EmlsrDlTxopTest::CheckAuxPhysSleepMode,
                                        this,
                                        m_staMacs[i],
                                        false);
                }
            }
        }
        break;

    default:;
    }
}

void
EmlsrDlTxopTest::DoSetup()
{
    // Channel switch delay should be less than the ICF padding duration, otherwise
    // DL TXOPs cannot be initiated on auxiliary links
    auto delay = std::min(MicroSeconds(100),
                          *std::min_element(m_paddingDelay.cbegin(), m_paddingDelay.cend()));
    Config::SetDefault("ns3::WifiPhy::ChannelSwitchDelay", TimeValue(MicroSeconds(75)));

    EmlsrOperationsTestBase::DoSetup();

    m_errorModel = CreateObject<ListErrorModel>();
    for (std::size_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
    {
        m_apMac->GetWifiPhy(linkId)->SetPostReceptionErrorModel(m_errorModel);
    }

    m_apMac->GetQosTxop(AC_BE)->SetTxopLimits(
        {MicroSeconds(3200), MicroSeconds(3200), MicroSeconds(3200)});

    if (m_nEmlsrStations + m_nNonEmlsrStations > 1)
    {
        auto muScheduler =
            CreateObjectWithAttributes<RrMultiUserScheduler>("EnableUlOfdma", BooleanValue(false));
        m_apMac->AggregateObject(muScheduler);
        for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
        {
            m_apMac->GetFrameExchangeManager(linkId)->GetAckManager()->SetAttribute(
                "DlMuAckSequenceType",
                EnumValue(WifiAcknowledgment::DL_MU_AGGREGATE_TF));
        }
    }
}

void
EmlsrDlTxopTest::StartTraffic()
{
    if (m_emlsrEnabledTime.IsZero())
    {
        // we are done with association and Block Ack agreement; we can now enable EMLSR mode
        m_lastAid = 0;
        EnableEmlsrMode();
        return;
    }

    // we are done with sending EML Operating Mode Notification frames. We can now generate
    // packets for all non-AP MLDs
    for (std::size_t i = 0; i < m_nEmlsrStations + m_nNonEmlsrStations; i++)
    {
        // when multiple non-AP MLDs are present, MU transmission are used. Given that the
        // available bandwidth decreases as the number of non-AP MLDs increases, compute the
        // number of packets to generate so that we always have two A-MPDUs per non-AP MLD
        std::size_t count = 8 / (m_nEmlsrStations + m_nNonEmlsrStations);
        m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(DOWNLINK, i, count, 450));
    }

    // in case of 2 EMLSR clients using no non-EMLSR link, generate one additional short
    // packet to each EMLSR client to test transition delay
    if (m_nEmlsrStations == 2 && m_apMac->GetNLinks() == m_emlsrLinks.size())
    {
        Simulator::Schedule(m_fe2to3delay, [&]() {
            m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(DOWNLINK, 0, 1, 40));
            m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(DOWNLINK, 1, 1, 40));
        });
    }

    // schedule the transmission of EML Operating Mode Notification frames to disable EMLSR mode
    // and the generation of other packets destined to the EMLSR clients
    for (std::size_t id = 0; id < m_nEmlsrStations; id++)
    {
        Simulator::Schedule(m_fe2to3delay + MilliSeconds(5 * (id + 1)), [=, this]() {
            m_staMacs.at(id)->GetEmlsrManager()->SetAttribute(
                "EmlsrLinkSet",
                AttributeContainerValue<UintegerValue>({}));
        });

        Simulator::Schedule(m_fe2to3delay + MilliSeconds(5 * (m_nEmlsrStations + 1)), [=, this]() {
            m_apMac->GetDevice()->GetNode()->AddApplication(
                GetApplication(DOWNLINK, id, 8 / m_nEmlsrStations, 650));
        });
    }
}

void
EmlsrDlTxopTest::EnableEmlsrMode()
{
    m_staMacs.at(m_lastAid)->GetEmlsrManager()->SetAttribute(
        "EmlsrLinkSet",
        AttributeContainerValue<UintegerValue>(m_emlsrLinks));
    m_lastAid++;
    Simulator::Schedule(MilliSeconds(5), [=, this]() {
        if (m_lastAid < m_nEmlsrStations)
        {
            // make the next STA send EML Notification frame
            EnableEmlsrMode();
            return;
        }
        // all stations enabled EMLSR mode; start traffic
        m_emlsrEnabledTime = Simulator::Now();
        StartTraffic();
    });
}

void
EmlsrDlTxopTest::CheckResults()
{
    auto psduIt = m_txPsdus.cbegin();

    // lambda to jump to the next QoS data frame or MU-RTS Trigger Frame transmitted
    // to an EMLSR client
    auto jumpToQosDataOrMuRts = [&]() {
        while (psduIt != m_txPsdus.cend() &&
               !psduIt->psduMap.cbegin()->second->GetHeader(0).IsQosData())
        {
            auto psdu = psduIt->psduMap.cbegin()->second;
            if (psdu->GetHeader(0).IsTrigger())
            {
                CtrlTriggerHeader trigger;
                psdu->GetPayload(0)->PeekHeader(trigger);
                if (trigger.IsMuRts())
                {
                    break;
                }
            }
            psduIt++;
        }
    };

    /**
     * Before enabling EMLSR mode, no MU-RTS TF should be sent. Four packets are generated
     * after association to trigger the establishment of a Block Ack agreement. The TXOP Limit
     * and the MCS are set such that two packets can be transmitted in a TXOP, hence we expect
     * that the AP MLD sends two A-MPDUs to each non-AP MLD.
     *
     * EMLSR client with EMLSR mode to be enabled on all links: after ML setup, all other links
     * stay in power save mode, hence BA establishment occurs on the same link.
     *
     *  [link 0]
     *  ───────────────────────────────────────────────────────────────────────────
     *                              | power save mode
     *
     *                   ┌─────┐      ┌─────┐                   ┌───┬───┐     ┌───┬───┐
     *            ┌───┐  │Assoc│      │ADDBA│             ┌───┐ │QoS│QoS│     │QoS│QoS│
     *  [link 1]  │ACK│  │Resp │      │ Req │             │ACK│ │ 0 │ 1 │     │ 2 │ 3 │
     *  ───┬─────┬┴───┴──┴─────┴┬───┬─┴─────┴┬───┬─┬─────┬┴───┴─┴───┴───┴┬──┬─┴───┴───┴┬──┬───
     *     │Assoc│              │ACK│        │ACK│ │ADDBA│               │BA│          │BA│
     *     │ Req │              └───┘        └───┘ │Resp │               └──┘          └──┘
     *     └─────┘                                 └─────┘
     *
     *  [link 2]
     *  ───────────────────────────────────────────────────────────────────────────
     *                              | power save mode
     *
     *
     * EMLSR client with EMLSR mode to be enabled on not all the links: after ML setup,
     * the other EMLSR links stay in power save mode, the non-EMLSR link (link 1) transitions
     * to active mode.
     *
     *                                             ┌─────┐                   ┌───┬───┐
     *                                      ┌───┐  │ADDBA│             ┌───┐ │QoS│QoS│
     *  [link 0 - non EMLSR]                │ACK│  │ Req │             │ACK│ │ 2 │ 3 │
     *  ──────────────────────────────┬────┬┴───┴──┴─────┴┬───┬─┬─────┬┴───┴─┴───┴───┴┬──┬─
     *                                │Data│              │ACK│ │ADDBA│               │BA│
     *                                │Null│              └───┘ │Resp │               └──┘
     *                                └────┘                    └─────┘
     *                   ┌─────┐                                       ┌───┬───┐
     *            ┌───┐  │Assoc│                                       │QoS│QoS│
     *  [link 1]  │ACK│  │Resp │                                       │ 0 │ 1 │
     *  ───┬─────┬┴───┴──┴─────┴┬───┬──────────────────────────────────┴───┴───┴┬──┬───────
     *     │Assoc│              │ACK│                                           │BA│
     *     │ Req │              └───┘                                           └──┘
     *     └─────┘
     *
     *  [link 2]
     *  ───────────────────────────────────────────────────────────────────────────
     *                              | power save mode
     *
     * Non-EMLSR client (not shown): after ML setup, all other links transition to active mode
     * by sending a Data Null frame; QoS data frame exchanges occur on two links simultaneously.
     */
    for (std::size_t i = 0; i < m_nEmlsrStations + m_nNonEmlsrStations; i++)
    {
        std::set<uint8_t> linkIds;

        jumpToQosDataOrMuRts();
        NS_TEST_ASSERT_MSG_EQ((psduIt != m_txPsdus.cend() &&
                               psduIt->psduMap.cbegin()->second->GetHeader(0).IsQosData()),
                              true,
                              "Expected at least one QoS data frame before enabling EMLSR mode");
        linkIds.insert(psduIt->linkId);
        const auto firstAmpduTxEnd =
            psduIt->startTx +
            WifiPhy::CalculateTxDuration(psduIt->psduMap,
                                         psduIt->txVector,
                                         m_staMacs[i]->GetWifiPhy(psduIt->linkId)->GetPhyBand());
        psduIt++;

        jumpToQosDataOrMuRts();
        NS_TEST_ASSERT_MSG_EQ((psduIt != m_txPsdus.cend() &&
                               psduIt->psduMap.cbegin()->second->GetHeader(0).IsQosData()),
                              true,
                              "Expected at least two QoS data frames before enabling EMLSR mode");
        linkIds.insert(psduIt->linkId);
        const auto secondAmpduTxStart = psduIt->startTx;
        psduIt++;

        /**
         * If this is an EMLSR client and there is no setup link other than the one used to
         * establish association that is not an EMLSR link, then the two A-MPDUs are sent one
         * after another on the link used to establish association.
         */
        auto setupLinks = m_staMacs[i]->GetSetupLinkIds();
        if (i < m_nEmlsrStations &&
            std::none_of(setupLinks.begin(), setupLinks.end(), [&](auto&& linkId) {
                return linkId != m_mainPhyId && m_emlsrLinks.count(linkId) == 0;
            }))
        {
            NS_TEST_EXPECT_MSG_EQ(linkIds.size(),
                                  1,
                                  "Expected both A-MPDUs to be sent on the same link");
            NS_TEST_EXPECT_MSG_EQ(*linkIds.begin(), +m_mainPhyId, "A-MPDUs sent on incorrect link");
            NS_TEST_EXPECT_MSG_LT(firstAmpduTxEnd,
                                  secondAmpduTxStart,
                                  "A-MPDUs are not sent one after another");
        }
        /**
         * Otherwise, the two A-MPDUs can be sent concurrently on two distinct links (may be
         * the link used to establish association and a non-EMLSR link).
         */
        else
        {
            NS_TEST_EXPECT_MSG_EQ(linkIds.size(),
                                  2,
                                  "Expected A-MPDUs to be sent on distinct links");
            NS_TEST_EXPECT_MSG_GT(firstAmpduTxEnd,
                                  secondAmpduTxStart,
                                  "A-MPDUs are not sent concurrently");
        }
    }

    /**
     * After enabling EMLSR mode, MU-RTS TF should only be sent on EMLSR links. After the exchange
     * of EML Operating Mode Notification frames, a number of packets are generated at the AP MLD
     * to prepare two A-MPDUs for each non-AP MLD.
     *
     * EMLSR client with EMLSR mode to be enabled on all links (A is the EMLSR client, B is the
     * non-EMLSR client):
     *                                      ┌─────┬─────┐
     *                                      │QoS 4│QoS 5│
     *                                      │ to A│ to A│
     *                            ┌───┐     ├─────┼─────┤
     *                            │MU │     │QoS 4│QoS 5│
     *  [link 0]                  │RTS│     │ to B│ to B│
     *  ──────────────────────────┴───┴┬───┬┴─────┴─────┴┬──┬────────────
     *                                 │CTS│             │BA│
     *                                 ├───┤             ├──┤
     *                                 │CTS│             │BA│
     *                                 └───┘             └──┘
     *                  ┌───┐      ┌─────┬─────┐
     *           ┌───┐  │EML│      │QoS 6│QoS 7│
     *  [link 1] │ACK│  │OM │      │ to B│ to B│
     *  ────┬───┬┴───┴──┴───┴┬───┬─┴─────┴─────┴┬──┬────────────────────────────────────
     *      │EML│            │ACK│              │BA│
     *      │OM │            └───┘              └──┘
     *      └───┘
     *                                                           ┌───┐     ┌─────┬─────┐
     *                                                           │MU │     │QoS 6│QoS 7│
     *  [link 2]                                                 │RTS│     │ to A│ to A│
     *  ─────────────────────────────────────────────────────────┴───┴┬───┬┴─────┴─────┴┬──┬─
     *                                                                │CTS│             │BA│
     *                                                                └───┘             └──┘
     *
     * EMLSR client with EMLSR mode to be enabled on not all the links (A is the EMLSR client,
     * B is the non-EMLSR client):
     *                             ┌─────┬─────┐
     *                             │QoS 4│QoS 5│
     *                             │ to A│ to A│
     *                             ├─────┼─────┤
     *                             │QoS 4│QoS 5│
     *  [link 0 - non EMLSR]       │ to B│ to B│
     *  ───────────────────────────┴─────┴─────┴┬──┬───────────────────────────
     *                                          │BA│
     *                                          ├──┤
     *                                          │BA│
     *                                          └──┘
     *                                       ┌─────┬─────┐
     *                                       │QoS 6│QoS 7│
     *                                       │ to A│ to A│
     *                  ┌───┐      ┌───┐     ├─────┼─────┤
     *           ┌───┐  │EML│      │MU │     │QoS 6│QoS 7│
     *  [link 1] │ACK│  │OM │      │RTS│     │ to B│ to B│
     *  ────┬───┬┴───┴──┴───┴┬───┬─┴───┴┬───┬┴─────┴─────┴┬──┬────────────
     *      │EML│            │ACK│      │CTS│             │BA│
     *      │OM │            └───┘      ├───┤             ├──┤
     *      └───┘                       │CTS│             │BA│
     *                                  └───┘             └──┘
     *
     *  [link 2]
     *  ────────────────────────────────────────────────────────────────────────────────
     */

    /// Store a QoS data frame or an MU-RTS TF followed by a QoS data frame
    using FrameExchange = std::list<decltype(psduIt)>;

    std::vector<std::list<FrameExchange>> frameExchanges(m_nEmlsrStations);

    // compute all frame exchanges involving EMLSR clients
    while (psduIt != m_txPsdus.cend())
    {
        jumpToQosDataOrMuRts();
        if (psduIt == m_txPsdus.cend())
        {
            break;
        }

        if (IsTrigger(psduIt->psduMap))
        {
            CtrlTriggerHeader trigger;
            psduIt->psduMap.cbegin()->second->GetPayload(0)->PeekHeader(trigger);
            // this is an MU-RTS TF starting a new frame exchange sequence; add it to all
            // the addressed EMLSR clients
            NS_TEST_ASSERT_MSG_EQ(trigger.IsMuRts(),
                                  true,
                                  "jumpToQosDataOrMuRts does not return TFs other than MU-RTS");
            for (const auto& userInfo : trigger)
            {
                for (std::size_t i = 0; i < m_nEmlsrStations; i++)
                {
                    if (m_staMacs.at(i)->GetAssociationId() == userInfo.GetAid12())
                    {
                        frameExchanges.at(i).emplace_back(FrameExchange{psduIt});
                        break;
                    }
                }
            }
            psduIt++;
            continue;
        }

        // we get here if psduIt points to a psduMap containing QoS data frame(s); find (if any)
        // the QoS data frame(s) addressed to EMLSR clients and add them to the appropriate
        // frame exchange sequence
        for (const auto& staIdPsduPair : psduIt->psduMap)
        {
            std::for_each_n(m_staMacs.cbegin(), m_nEmlsrStations, [&](auto&& staMac) {
                if (!staMac->GetLinkIdByAddress(staIdPsduPair.second->GetAddr1()))
                {
                    // not addressed to this non-AP MLD
                    return;
                }
                // a QoS data frame starts a new frame exchange sequence if there is no previous
                // MU-RTS TF that has been sent on the same link and is not already followed by
                // a QoS data frame
                std::size_t id = staMac->GetDevice()->GetNode()->GetId() - 1;
                for (auto& frameExchange : frameExchanges.at(id))
                {
                    if (IsTrigger(frameExchange.front()->psduMap) &&
                        frameExchange.front()->linkId == psduIt->linkId &&
                        frameExchange.size() == 1)
                    {
                        auto it = std::next(frameExchange.front());
                        while (it != m_txPsdus.end())
                        {
                            // stop at the first frame other than CTS sent on this link
                            if (it->linkId == psduIt->linkId &&
                                !it->psduMap.begin()->second->GetHeader(0).IsCts())
                            {
                                break;
                            }
                            ++it;
                        }
                        if (it == psduIt)
                        {
                            // the QoS data frame actually followed the MU-RTS TF
                            frameExchange.emplace_back(psduIt);
                            return;
                        }
                    }
                }
                frameExchanges.at(id).emplace_back(FrameExchange{psduIt});
            });
        }
        psduIt++;
    }

    /**
     * Let's focus on the first two frame exchanges for each EMLSR clients. If all setup links are
     * EMLSR links, both frame exchanges are protected by MU-RTS TF and occur one after another.
     * Otherwise, one frame exchange occurs on the non-EMLSR link and is not protected by
     * MU-RTS TF; the other frame exchange occurs on an EMLSR link and is protected by MU-RTS TF.
     */
    for (std::size_t i = 0; i < m_nEmlsrStations; i++)
    {
        NS_TEST_EXPECT_MSG_GT_OR_EQ(frameExchanges.at(i).size(),
                                    2,
                                    "Expected at least 2 frame exchange sequences "
                                        << "involving EMLSR client " << i);

        auto firstExchangeIt = frameExchanges.at(i).begin();
        auto secondExchangeIt = std::next(firstExchangeIt);

        const auto firstAmpduTxEnd =
            firstExchangeIt->back()->startTx +
            WifiPhy::CalculateTxDuration(
                firstExchangeIt->back()->psduMap,
                firstExchangeIt->back()->txVector,
                m_staMacs[i]->GetWifiPhy(firstExchangeIt->back()->linkId)->GetPhyBand());
        const auto secondAmpduTxStart = secondExchangeIt->front()->startTx;

        if (m_staMacs[i]->GetNLinks() == m_emlsrLinks.size())
        {
            // all links are EMLSR links
            NS_TEST_EXPECT_MSG_EQ(IsTrigger(firstExchangeIt->front()->psduMap),
                                  true,
                                  "Expected an MU-RTS TF as ICF of first frame exchange sequence");
            NS_TEST_EXPECT_MSG_EQ(
                firstExchangeIt->back()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
                true,
                "Expected a QoS data frame in the first frame exchange sequence");

            NS_TEST_EXPECT_MSG_EQ(IsTrigger(secondExchangeIt->front()->psduMap),
                                  true,
                                  "Expected an MU-RTS TF as ICF of second frame exchange sequence");
            NS_TEST_EXPECT_MSG_EQ(
                secondExchangeIt->back()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
                true,
                "Expected a QoS data frame in the second frame exchange sequence");

            NS_TEST_EXPECT_MSG_LT(firstAmpduTxEnd,
                                  secondAmpduTxStart,
                                  "A-MPDUs are not sent one after another");
        }
        else
        {
            std::vector<uint8_t> nonEmlsrIds;
            auto setupLinks = m_staMacs[i]->GetSetupLinkIds();
            std::set_difference(setupLinks.begin(),
                                setupLinks.end(),
                                m_emlsrLinks.begin(),
                                m_emlsrLinks.end(),
                                std::back_inserter(nonEmlsrIds));
            NS_TEST_ASSERT_MSG_EQ(nonEmlsrIds.size(), 1, "Unexpected number of non-EMLSR links");

            auto nonEmlsrLinkExchangeIt = firstExchangeIt->front()->linkId == nonEmlsrIds[0]
                                              ? firstExchangeIt
                                              : secondExchangeIt;
            NS_TEST_EXPECT_MSG_EQ(IsTrigger(nonEmlsrLinkExchangeIt->front()->psduMap),
                                  false,
                                  "Did not expect an MU-RTS TF as ICF on non-EMLSR link");
            NS_TEST_EXPECT_MSG_EQ(
                nonEmlsrLinkExchangeIt->front()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
                true,
                "Expected a QoS data frame on the non-EMLSR link");

            auto emlsrLinkExchangeIt =
                nonEmlsrLinkExchangeIt == firstExchangeIt ? secondExchangeIt : firstExchangeIt;
            NS_TEST_EXPECT_MSG_NE(+emlsrLinkExchangeIt->front()->linkId,
                                  +nonEmlsrIds[0],
                                  "Expected this exchange not to occur on non-EMLSR link");
            NS_TEST_EXPECT_MSG_EQ(IsTrigger(emlsrLinkExchangeIt->front()->psduMap),
                                  true,
                                  "Expected an MU-RTS TF as ICF on the EMLSR link");
            NS_TEST_EXPECT_MSG_EQ(
                emlsrLinkExchangeIt->back()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
                true,
                "Expected a QoS data frame on the EMLSR link");

            NS_TEST_EXPECT_MSG_GT(firstAmpduTxEnd,
                                  secondAmpduTxStart,
                                  "A-MPDUs are not sent concurrently");
        }

        // we are done with processing the first two frame exchanges, remove them
        frameExchanges.at(i).erase(firstExchangeIt);
        frameExchanges.at(i).erase(secondExchangeIt);
    }

    /**
     * A and B are two EMLSR clients. No ICF before the second QoS data frame because B
     * has not switched to listening mode. ICF is sent before the third QoS data frame because
     * A has switched to listening mode. C is a non-EMLSR client.
     *
     *                        ┌─────┐          A switches to listening
     *                        │QoS x│          after transition delay
     *                        │ to A│          |
     *              ┌───┐     ├─────┤    ┌─────┐
     *              │MU │     │QoS x│    │QoS y│
     *  [link 0]    │RTS│     │ to B│    │ to B│
     *  ────────────┴───┴┬───┬┴─────┴┬──┬┴─────┴┬──┬────────────
     *                   │CTS│       │BA│       │BA│
     *                   ├───┤       ├──┤       └──┘
     *                   │CTS│       │BA│
     *                   └───┘       └──┘        AP continues the TXOP     A switches to listening
     *                                             after PIFS recovery      after transition delay
     *                                                                │                       │
     *                                 ┌─────┐    ┌───┐     ┌─────┐   │┌───┐              ┌───┐
     *                                 │QoS z│    │MU │     │QoS x│   ││MU │     ┌───┐    │CF-│
     *  [link 1]                       │ to C│    │RTS│     │ to A│   ││RTS│     │BAR│    │End│
     *  ───────────────────────────────┴─────┴┬──┬┴───┴┬───┬┴─────┴┬──┬┴───┴┬───┬┴───┴┬──┬┴───┴─
     *                                        │BA│     │CTS│       │BA│     │CTS│     │BA│
     *                                        └──┘     └───┘       └──x     └───┘     └──┘
     */
    if (m_nEmlsrStations == 2 && m_apMac->GetNLinks() == m_emlsrLinks.size())
    {
        // the following checks are only done with 2 EMLSR clients having no non-EMLSR link
        for (std::size_t i = 0; i < m_nEmlsrStations; i++)
        {
            NS_TEST_EXPECT_MSG_GT_OR_EQ(frameExchanges.at(i).size(),
                                        2,
                                        "Expected at least 2 frame exchange sequences "
                                            << "involving EMLSR client " << i);
            // the first frame exchange must start with an ICF
            auto firstExchangeIt = frameExchanges.at(i).begin();

            NS_TEST_EXPECT_MSG_EQ(IsTrigger(firstExchangeIt->front()->psduMap),
                                  true,
                                  "Expected an MU-RTS TF as ICF of first frame exchange sequence");
            NS_TEST_EXPECT_MSG_EQ(
                firstExchangeIt->back()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
                true,
                "Expected a QoS data frame in the first frame exchange sequence");
        }

        // the second frame exchange is the one that starts first
        auto secondExchangeIt = std::next(frameExchanges.at(0).begin())->front()->startTx <
                                        std::next(frameExchanges.at(1).begin())->front()->startTx
                                    ? std::next(frameExchanges.at(0).begin())
                                    : std::next(frameExchanges.at(1).begin());
        decltype(secondExchangeIt) thirdExchangeIt;
        std::size_t thirdExchangeStaId;

        if (secondExchangeIt == std::next(frameExchanges.at(0).begin()))
        {
            thirdExchangeIt = std::next(frameExchanges.at(1).begin());
            thirdExchangeStaId = 1;
        }
        else
        {
            thirdExchangeIt = std::next(frameExchanges.at(0).begin());
            thirdExchangeStaId = 0;
        }

        // the second frame exchange is not protected by the ICF and starts a SIFS after the end
        // of the previous one
        NS_TEST_EXPECT_MSG_EQ(IsTrigger(secondExchangeIt->front()->psduMap),
                              false,
                              "Expected no ICF for the second frame exchange sequence");
        NS_TEST_EXPECT_MSG_EQ(
            secondExchangeIt->front()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
            true,
            "Expected a QoS data frame in the second frame exchange sequence");

        // the first two frame exchanges occur on the same link
        NS_TEST_EXPECT_MSG_EQ(+secondExchangeIt->front()->linkId,
                              +frameExchanges.at(0).begin()->front()->linkId,
                              "Expected the first two frame exchanges to occur on the same link");

        auto bAckRespIt = std::prev(secondExchangeIt->front());
        NS_TEST_EXPECT_MSG_EQ(bAckRespIt->psduMap.cbegin()->second->GetHeader(0).IsBlockAck(),
                              true,
                              "Expected a BlockAck response before the second frame exchange");
        auto bAckRespTxEnd =
            bAckRespIt->startTx +
            WifiPhy::CalculateTxDuration(bAckRespIt->psduMap,
                                         bAckRespIt->txVector,
                                         m_apMac->GetWifiPhy(bAckRespIt->linkId)->GetPhyBand());

        // the second frame exchange starts a SIFS after the previous one
        NS_TEST_EXPECT_MSG_EQ(
            bAckRespTxEnd + m_apMac->GetWifiPhy(bAckRespIt->linkId)->GetSifs(),
            secondExchangeIt->front()->startTx,
            "Expected the second frame exchange to start a SIFS after the first one");

        // the third frame exchange is protected by MU-RTS and occurs on a different link
        NS_TEST_EXPECT_MSG_EQ(IsTrigger(thirdExchangeIt->front()->psduMap),
                              true,
                              "Expected an MU-RTS as ICF for the third frame exchange sequence");
        NS_TEST_EXPECT_MSG_EQ(
            thirdExchangeIt->back()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
            true,
            "Expected a QoS data frame in the third frame exchange sequence");

        NS_TEST_EXPECT_MSG_NE(
            +secondExchangeIt->front()->linkId,
            +thirdExchangeIt->front()->linkId,
            "Expected the second and third frame exchanges to occur on distinct links");

        auto secondQosIt = secondExchangeIt->front();
        auto secondQosTxEnd =
            secondQosIt->startTx +
            WifiPhy::CalculateTxDuration(secondQosIt->psduMap,
                                         secondQosIt->txVector,
                                         m_apMac->GetWifiPhy(secondQosIt->linkId)->GetPhyBand());

        NS_TEST_EXPECT_MSG_GT_OR_EQ(thirdExchangeIt->front()->startTx,
                                    secondQosTxEnd + m_transitionDelay.at(thirdExchangeStaId),
                                    "Transmission started before transition delay");

        // the BlockAck of the third frame exchange is not received correctly, so there should be
        // another frame exchange
        NS_TEST_EXPECT_MSG_EQ((thirdExchangeIt != frameExchanges.at(thirdExchangeStaId).end()),
                              true,
                              "Expected a fourth frame exchange");
        auto fourthExchangeIt = std::next(thirdExchangeIt);

        // the fourth frame exchange is protected by MU-RTS
        NS_TEST_EXPECT_MSG_EQ(IsTrigger(fourthExchangeIt->front()->psduMap),
                              true,
                              "Expected an MU-RTS as ICF for the fourth frame exchange sequence");

        bAckRespIt = std::prev(fourthExchangeIt->front());
        NS_TEST_EXPECT_MSG_EQ(bAckRespIt->psduMap.cbegin()->second->GetHeader(0).IsBlockAck(),
                              true,
                              "Expected a BlockAck response before the fourth frame exchange");
        auto phy = m_apMac->GetWifiPhy(bAckRespIt->linkId);
        bAckRespTxEnd = bAckRespIt->startTx + WifiPhy::CalculateTxDuration(bAckRespIt->psduMap,
                                                                           bAckRespIt->txVector,
                                                                           phy->GetPhyBand());
        auto timeout = phy->GetSifs() + phy->GetSlot() + MicroSeconds(20);

        // the fourth frame exchange starts a PIFS after the previous one because the AP
        // performs PIFS recovery (the initial frame in the TXOP was successfully received by
        // a non-EMLSR client)
        NS_TEST_EXPECT_MSG_GT_OR_EQ(fourthExchangeIt->front()->startTx,
                                    bAckRespTxEnd + phy->GetPifs(),
                                    "Transmission started less than a PIFS after BlockAck");
        NS_TEST_EXPECT_MSG_LT(fourthExchangeIt->front()->startTx,
                              bAckRespTxEnd + phy->GetPifs() +
                                  MicroSeconds(1) /* propagation delay upper bound */,
                              "Transmission started too much time after BlockAck");

        auto bAckReqIt = std::next(fourthExchangeIt->front(), 2);
        NS_TEST_EXPECT_MSG_EQ(bAckReqIt->psduMap.cbegin()->second->GetHeader(0).IsBlockAckReq(),
                              true,
                              "Expected a BlockAck request in the fourth frame exchange");

        // we are done with processing the frame exchanges, remove them (two frame exchanges
        // per EMLSR client, plus the last one)
        frameExchanges.at(0).pop_front();
        frameExchanges.at(0).pop_front();
        frameExchanges.at(1).pop_front();
        frameExchanges.at(1).pop_front();
        frameExchanges.at(thirdExchangeStaId).pop_front();
    }

    /**
     * After disabling EMLSR mode, no MU-RTS TF should be sent. After the exchange of
     * EML Operating Mode Notification frames, a number of packets are generated at the AP MLD
     * to prepare two A-MPDUs for each EMLSR client.
     *
     * EMLSR client with EMLSR mode to be enabled on all links (A is the EMLSR client, B is the
     * non-EMLSR client):
     *
     *  [link 0]                            | power save mode
     *  ────────────────────────────────────────────────────────
     *                                        ┌─────┬─────┐        ┌──────┬──────┐
     *                                        │QoS 8│QoS 9│        │QoS 10│QoS 11│
     *                                        │ to A│ to A│        │ to A │ to A │
     *                  ┌───┐     ┌───┐       ├─────┼─────┤        ├──────┼──────┤
     *           ┌───┐  │MU │     │EML│       │QoS 8│QoS 9│        │QoS 10│QoS 11│
     *  [link 1] │ACK│  │RTS│     │OM │       │ to B│ to B│        │ to B │ to B │
     *  ────┬───┬┴───┴──┴───┴┬───┬┴───┴┬───┬──┴─────┴─────┴┬──┬────┴──────┴──────┴┬──┬─────
     *      │EML│            │CTS│     │ACK│               │BA│                   │BA│
     *      │OM │            └───┘     └───┘               ├──┤                   ├──┤
     *      └───┘                                          │BA│                   │BA│
     *                                                     └──┘                   └──┘
     *
     *  [link 2]                            | power save mode
     *  ────────────────────────────────────────────────────────────────────────────
     *
     *
     * EMLSR client with EMLSR mode to be enabled on not all the links (A is the EMLSR client,
     * B is the non-EMLSR client):
     *                                           ┌─────┬─────┐
     *                                           │QoS 8│QoS 9│
     *                                           │ to A│ to A│
     *                                           ├─────┼─────┤
     *                                           │QoS 8│QoS 9│
     *  [link 0 - non EMLSR]                     │ to B│ to B│
     *  ─────────────────────────────────────────┴─────┴─────┴┬──┬─────────────
     *                                                        │BA│
     *                                                        ├──┤
     *                                                        │BA│
     *                                                        └──┘
     *                                        ┌──────┬──────┐
     *                                        │QoS 10│QoS 11│
     *                                        │ to A │ to A │
     *                  ┌───┐     ┌───┐       ├──────┼──────┤
     *           ┌───┐  │MU │     │EML│       │QoS 10│QoS 11│
     *  [link 1] │ACK│  │RTS│     │OM │       │ to B │ to B │
     *  ────┬───┬┴───┴──┴───┴┬───┬┴───┴┬───┬──┴──────┴──────┴┬──┬─────
     *      │EML│            │CTS│     │ACK│                 │BA│
     *      │OM │            └───┘     └───┘                 ├──┤
     *      └───┘                                            │BA│
     *                                                       └──┘
     *
     *  [link 2]                            | power save mode
     *  ────────────────────────────────────────────────────────────────────────────
     *
     */

    // for each EMLSR client, there should be a frame exchange with ICF and no data frame
    // (ICF protects the EML Notification response) if the EML Notification response is sent
    // while EMLSR mode is still enabled and two frame exchanges with data frames
    for (std::size_t i = 0; i < m_nEmlsrStations; i++)
    {
        // the default EMLSR Manager requests to send EML Notification frames on the link where
        // the main PHY is operating; if EMLSR mode is still enabled on this link when the AP MLD
        // sends the EML Notification response, the latter is protected by an ICF
        auto exchangeIt = frameExchanges.at(i).cbegin();

        auto linkIdOpt = m_staMacs[i]->GetLinkForPhy(m_mainPhyId);
        NS_TEST_ASSERT_MSG_EQ(linkIdOpt.has_value(),
                              true,
                              "Didn't find a link on which the main PHY is operating");

        if (IsTrigger(exchangeIt->front()->psduMap))
        {
            NS_TEST_EXPECT_MSG_EQ(+exchangeIt->front()->linkId,
                                  +linkIdOpt.value(),
                                  "ICF was not sent on the expected link");
            NS_TEST_EXPECT_MSG_EQ(exchangeIt->size(),
                                  1,
                                  "Expected no data frame in the first frame exchange sequence");
            frameExchanges.at(i).pop_front();
        }

        NS_TEST_EXPECT_MSG_GT_OR_EQ(frameExchanges.at(i).size(),
                                    2,
                                    "Expected at least 2 frame exchange sequences "
                                        << "involving EMLSR client " << i);

        auto firstExchangeIt = frameExchanges.at(i).cbegin();
        auto secondExchangeIt = std::next(firstExchangeIt);

        const auto firstAmpduTxEnd =
            firstExchangeIt->back()->startTx +
            WifiPhy::CalculateTxDuration(
                firstExchangeIt->back()->psduMap,
                firstExchangeIt->back()->txVector,
                m_staMacs[i]->GetWifiPhy(firstExchangeIt->back()->linkId)->GetPhyBand());
        const auto secondAmpduTxStart = secondExchangeIt->front()->startTx;

        NS_TEST_EXPECT_MSG_EQ(
            firstExchangeIt->front()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
            true,
            "Expected a QoS data frame in the first frame exchange sequence");
        NS_TEST_EXPECT_MSG_EQ(firstExchangeIt->size(),
                              1,
                              "Expected one frame only in the first frame exchange sequence");

        NS_TEST_EXPECT_MSG_EQ(
            secondExchangeIt->front()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
            true,
            "Expected a QoS data frame in the second frame exchange sequence");
        NS_TEST_EXPECT_MSG_EQ(secondExchangeIt->size(),
                              1,
                              "Expected one frame only in the second frame exchange sequence");

        if (m_staMacs[i]->GetNLinks() == m_emlsrLinks.size())
        {
            // all links are EMLSR links: the two QoS data frames are sent one after another on
            // the link used for sending EML OMN
            NS_TEST_EXPECT_MSG_EQ(
                +firstExchangeIt->front()->linkId,
                +linkIdOpt.value(),
                "First frame exchange expected to occur on link used to send EML OMN");

            NS_TEST_EXPECT_MSG_EQ(
                +secondExchangeIt->front()->linkId,
                +linkIdOpt.value(),
                "Second frame exchange expected to occur on link used to send EML OMN");

            NS_TEST_EXPECT_MSG_LT(firstAmpduTxEnd,
                                  secondAmpduTxStart,
                                  "A-MPDUs are not sent one after another");
        }
        else
        {
            // the two QoS data frames are sent concurrently on distinct links
            NS_TEST_EXPECT_MSG_NE(+firstExchangeIt->front()->linkId,
                                  +secondExchangeIt->front()->linkId,
                                  "Frame exchanges expected to occur on distinct links");

            NS_TEST_EXPECT_MSG_GT(firstAmpduTxEnd,
                                  secondAmpduTxStart,
                                  "A-MPDUs are not sent concurrently");
        }
    }
}

void
EmlsrDlTxopTest::CheckPmModeAfterAssociation(const Mac48Address& address)
{
    std::optional<std::size_t> staId;
    for (std::size_t id = 0; id < m_nEmlsrStations + m_nNonEmlsrStations; id++)
    {
        if (m_staMacs.at(id)->GetLinkIdByAddress(address))
        {
            staId = id;
            break;
        }
    }
    NS_TEST_ASSERT_MSG_EQ(staId.has_value(), true, "Not an address of a non-AP MLD " << address);

    // check that all EMLSR links (but the link used for ML setup) of the EMLSR clients
    // are considered to be in power save mode by the AP MLD; all the other links have
    // transitioned to active mode instead
    for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
    {
        bool psModeExpected =
            *staId < m_nEmlsrStations && linkId != m_mainPhyId && m_emlsrLinks.count(linkId) == 1;
        auto addr = m_staMacs.at(*staId)->GetAddress();
        auto psMode = m_apMac->GetWifiRemoteStationManager(linkId)->IsInPsMode(addr);
        NS_TEST_EXPECT_MSG_EQ(psMode,
                              psModeExpected,
                              "EMLSR link " << +linkId << " of EMLSR client " << *staId
                                            << " not in " << (psModeExpected ? "PS" : "active")
                                            << " mode");
        // check that AP is blocking transmission of QoS data frames on this link
        CheckBlockedLink(m_apMac,
                         addr,
                         linkId,
                         WifiQueueBlockedReason::POWER_SAVE_MODE,
                         psModeExpected,
                         "Checking PM mode after association on AP MLD for EMLSR client " +
                             std::to_string(*staId),
                         false);
    }
}

void
EmlsrDlTxopTest::CheckApEmlNotificationFrame(Ptr<const WifiMpdu> mpdu,
                                             const WifiTxVector& txVector,
                                             uint8_t linkId)
{
    // the AP is replying to a received EMLSR Notification frame
    auto pkt = mpdu->GetPacket()->Copy();
    const auto& hdr = mpdu->GetHeader();
    WifiActionHeader::Remove(pkt);
    MgtEmlOmn frame;
    pkt->RemoveHeader(frame);

    std::optional<std::size_t> staId;
    for (std::size_t id = 0; id < m_nEmlsrStations; id++)
    {
        if (m_staMacs.at(id)->GetFrameExchangeManager(linkId)->GetAddress() == hdr.GetAddr1())
        {
            staId = id;
            break;
        }
    }
    NS_TEST_ASSERT_MSG_EQ(staId.has_value(),
                          true,
                          "Not an address of an EMLSR client " << hdr.GetAddr1());

    // The EMLSR mode change occurs a Transition Timeout after the end of the PPDU carrying the Ack
    auto phy = m_apMac->GetWifiPhy(linkId);
    auto txDuration =
        WifiPhy::CalculateTxDuration(mpdu->GetSize() + 4, // A-MPDU Subframe header size
                                     txVector,
                                     phy->GetPhyBand());
    WifiTxVector ackTxVector =
        m_staMacs.at(*staId)->GetWifiRemoteStationManager(linkId)->GetAckTxVector(hdr.GetAddr2(),
                                                                                  txVector);
    auto ackDuration = WifiPhy::CalculateTxDuration(GetAckSize() + 4, // A-MPDU Subframe header
                                                    ackTxVector,
                                                    phy->GetPhyBand());

    Simulator::Schedule(txDuration + phy->GetSifs() + ackDuration, [=, this]() {
        if (frame.m_emlControl.emlsrMode == 1)
        {
            // EMLSR mode enabled. Check that all EMLSR links of the EMLSR clients are considered
            // to be in active mode by the AP MLD
            for (const auto linkId : m_emlsrLinks)
            {
                auto addr = m_staMacs.at(*staId)->GetAddress();
                auto psMode = m_apMac->GetWifiRemoteStationManager(linkId)->IsInPsMode(addr);
                NS_TEST_EXPECT_MSG_EQ(psMode,
                                      false,
                                      "EMLSR link " << +linkId << " of EMLSR client " << *staId
                                                    << " not in active mode");
                // check that AP is not blocking transmission of QoS data frames on this link
                CheckBlockedLink(
                    m_apMac,
                    addr,
                    linkId,
                    WifiQueueBlockedReason::POWER_SAVE_MODE,
                    false,
                    "Checking EMLSR links on AP MLD after EMLSR mode is enabled on EMLSR client " +
                        std::to_string(*staId),
                    false);
            }
        }
        else
        {
            // EMLSR mode disabled. Check that all EMLSR links (but the link used to send the
            // EML Notification frame) of the EMLSR clients are considered to be in power save
            // mode by the AP MLD; the other links are in active mode
            for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
            {
                bool psModeExpected = id != linkId && m_emlsrLinks.count(id) == 1;
                auto addr = m_staMacs.at(*staId)->GetAddress();
                auto psMode = m_apMac->GetWifiRemoteStationManager(id)->IsInPsMode(addr);
                NS_TEST_EXPECT_MSG_EQ(psMode,
                                      psModeExpected,
                                      "EMLSR link "
                                          << +id << " of EMLSR client " << *staId << " not in "
                                          << (psModeExpected ? "PS" : "active") << " mode");
                // check that AP is blocking transmission of QoS data frames on this link
                CheckBlockedLink(
                    m_apMac,
                    addr,
                    id,
                    WifiQueueBlockedReason::POWER_SAVE_MODE,
                    psModeExpected,
                    "Checking links on AP MLD after EMLSR mode is disabled on EMLSR client " +
                        std::to_string(*staId),
                    false);
            }
        }
    });
}

void
EmlsrDlTxopTest::CheckStaEmlNotificationFrame(Ptr<const WifiMpdu> mpdu,
                                              const WifiTxVector& txVector,
                                              uint8_t linkId)
{
    // an EMLSR client is sending an EMLSR Notification frame
    auto pkt = mpdu->GetPacket()->Copy();
    const auto& hdr = mpdu->GetHeader();
    WifiActionHeader::Remove(pkt);
    MgtEmlOmn frame;
    pkt->RemoveHeader(frame);

    std::optional<std::size_t> staId;
    for (std::size_t id = 0; id < m_nEmlsrStations; id++)
    {
        if (m_staMacs.at(id)->GetFrameExchangeManager(linkId)->GetAddress() == hdr.GetAddr2())
        {
            staId = id;
            break;
        }
    }
    NS_TEST_ASSERT_MSG_EQ(staId.has_value(),
                          true,
                          "Not an address of an EMLSR client " << hdr.GetAddr1());

    auto phy = m_staMacs.at(*staId)->GetWifiPhy(linkId);
    auto txDuration = WifiPhy::CalculateTxDuration(mpdu->GetSize(), txVector, phy->GetPhyBand());
    auto ackTxVector =
        m_apMac->GetWifiRemoteStationManager(linkId)->GetAckTxVector(hdr.GetAddr2(), txVector);
    auto ackDuration = WifiPhy::CalculateTxDuration(GetAckSize(), ackTxVector, phy->GetPhyBand());
    auto cfEndDuration = WifiPhy::CalculateTxDuration(
        Create<WifiPsdu>(Create<Packet>(), WifiMacHeader(WIFI_MAC_CTL_END)),
        m_staMacs.at(*staId)->GetWifiRemoteStationManager(linkId)->GetRtsTxVector(
            Mac48Address::GetBroadcast(),
            txVector.GetChannelWidth()),
        phy->GetPhyBand());

    if (frame.m_emlControl.emlsrMode != 0)
    {
        return;
    }

    // EMLSR mode disabled
    auto timeToCfEnd = txDuration + phy->GetSifs() + ackDuration + phy->GetSifs() + cfEndDuration;

    // before the end of the CF-End frame, this link only is not blocked on both the
    // EMLSR client and the AP MLD
    Simulator::Schedule(timeToCfEnd - MicroSeconds(1), [=, this]() {
        for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
        {
            CheckBlockedLink(m_staMacs.at(*staId),
                             m_apMac->GetAddress(),
                             id,
                             WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                             id != linkId && m_staMacs.at(*staId)->IsEmlsrLink(id),
                             "Checking links on EMLSR client " + std::to_string(*staId) +
                                 " before the end of CF-End frame");
            CheckBlockedLink(m_apMac,
                             m_staMacs.at(*staId)->GetAddress(),
                             id,
                             WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                             id != linkId && m_staMacs.at(*staId)->IsEmlsrLink(id),
                             "Checking links of EMLSR client " + std::to_string(*staId) +
                                 " on the AP MLD before the end of CF-End frame");
        }
    });
    // after the end of the CF-End frame, all links for the EMLSR client are blocked on the
    // AP MLD
    Simulator::Schedule(timeToCfEnd + MicroSeconds(1), [=, this]() {
        for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
        {
            if (m_staMacs.at(*staId)->IsEmlsrLink(id))
            {
                CheckBlockedLink(
                    m_apMac,
                    m_staMacs.at(*staId)->GetAddress(),
                    id && m_staMacs.at(*staId)->IsEmlsrLink(id),
                    WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                    true,
                    "Checking links of EMLSR client " + std::to_string(*staId) +
                        " are all blocked on the AP MLD right after the end of CF-End");
            }
        }
    });
    // before the end of the transition delay, all links for the EMLSR client are still
    // blocked on the AP MLD
    Simulator::Schedule(timeToCfEnd + m_transitionDelay.at(*staId) - MicroSeconds(1), [=, this]() {
        for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
        {
            if (m_staMacs.at(*staId)->IsEmlsrLink(id))
            {
                CheckBlockedLink(m_apMac,
                                 m_staMacs.at(*staId)->GetAddress(),
                                 id,
                                 WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                                 true,
                                 "Checking links of EMLSR client " + std::to_string(*staId) +
                                     " are all blocked on the AP MLD before the end of "
                                     "transition delay");
            }
        }
    });
    // immediately after the transition delay, all links for the EMLSR client are unblocked
    Simulator::Schedule(timeToCfEnd + m_transitionDelay.at(*staId) + MicroSeconds(1), [=, this]() {
        for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
        {
            if (m_staMacs.at(*staId)->IsEmlsrLink(id))
            {
                CheckBlockedLink(m_apMac,
                                 m_staMacs.at(*staId)->GetAddress(),
                                 id,
                                 WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                                 false,
                                 "Checking links of EMLSR client " + std::to_string(*staId) +
                                     " are all unblocked on the AP MLD after the transition delay");
            }
        }
    });
}

void
EmlsrDlTxopTest::CheckInitialControlFrame(Ptr<const WifiMpdu> mpdu,
                                          const WifiTxVector& txVector,
                                          uint8_t linkId)
{
    CtrlTriggerHeader trigger;
    mpdu->GetPacket()->PeekHeader(trigger);
    if (!trigger.IsMuRts())
    {
        return;
    }

    NS_TEST_EXPECT_MSG_EQ(m_emlsrEnabledTime.IsStrictlyPositive(),
                          true,
                          "Did not expect an ICF before enabling EMLSR mode");

    NS_TEST_EXPECT_MSG_LT(txVector.GetPreambleType(),
                          WIFI_PREAMBLE_HT_MF,
                          "Unexpected preamble type for the Initial Control frame");
    auto rate = txVector.GetMode().GetDataRate(txVector);
    NS_TEST_EXPECT_MSG_EQ((rate == 6e6 || rate == 12e6 || rate == 24e6),
                          true,
                          "Unexpected rate for the Initial Control frame: " << rate);

    bool found = false;
    Time maxPaddingDelay{};

    for (const auto& userInfo : trigger)
    {
        auto addr = m_apMac->GetMldOrLinkAddressByAid(userInfo.GetAid12());
        NS_TEST_ASSERT_MSG_EQ(addr.has_value(),
                              true,
                              "AID " << userInfo.GetAid12() << " not found");

        if (m_apMac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(*addr))
        {
            found = true;

            for (std::size_t i = 0; i < m_nEmlsrStations; i++)
            {
                if (m_staMacs.at(i)->GetAddress() == *addr)
                {
                    maxPaddingDelay = Max(maxPaddingDelay, m_paddingDelay.at(i));
                    break;
                }
            }

            // check that the AP has blocked transmission on all other EMLSR links
            for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
            {
                if (!m_apMac->GetWifiRemoteStationManager(id)->GetEmlsrEnabled(*addr))
                {
                    continue;
                }

                CheckBlockedLink(m_apMac,
                                 *addr,
                                 id,
                                 WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                 id != linkId,
                                 "Checking that AP blocked transmissions on all other EMLSR "
                                 "links after sending ICF to client with AID=" +
                                     std::to_string(userInfo.GetAid12()),
                                 false);
            }
        }
    }

    NS_TEST_EXPECT_MSG_EQ(found, true, "Expected ICF to be addressed to at least an EMLSR client");

    auto txDuration = WifiPhy::CalculateTxDuration(mpdu->GetSize(),
                                                   txVector,
                                                   m_apMac->GetWifiPhy(linkId)->GetPhyBand());

    if (maxPaddingDelay.IsStrictlyPositive())
    {
        // compare the TX duration of this Trigger Frame to that of the Trigger Frame with no
        // padding added
        trigger.SetPaddingSize(0);
        auto pkt = Create<Packet>();
        pkt->AddHeader(trigger);
        auto txDurationWithout =
            WifiPhy::CalculateTxDuration(Create<WifiPsdu>(pkt, mpdu->GetHeader()),
                                         txVector,
                                         m_apMac->GetWifiPhy(linkId)->GetPhyBand());

        NS_TEST_EXPECT_MSG_EQ(txDuration,
                              txDurationWithout + maxPaddingDelay,
                              "Unexpected TX duration of the MU-RTS TF with padding "
                                  << maxPaddingDelay.As(Time::US));
    }

    // check that the EMLSR clients have blocked transmissions on other links, switched their main
    // PHY (if needed) and have put aux PHYs to sleep after receiving this ICF
    for (const auto& userInfo : trigger)
    {
        for (std::size_t i = 0; i < m_nEmlsrStations; i++)
        {
            if (m_staMacs[i]->GetAssociationId() != userInfo.GetAid12())
            {
                continue;
            }

            const auto mainPhyLinkId = m_staMacs[i]->GetLinkForPhy(m_mainPhyId);

            Simulator::Schedule(txDuration + NanoSeconds(5), [=, this]() {
                for (uint8_t id = 0; id < m_staMacs[i]->GetNLinks(); id++)
                {
                    // non-EMLSR links or links on which ICF is received are not blocked
                    CheckBlockedLink(m_staMacs[i],
                                     m_apMac->GetAddress(),
                                     id,
                                     WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                     id != linkId && m_staMacs[i]->IsEmlsrLink(id),
                                     "Checking EMLSR links on EMLSR client " + std::to_string(i) +
                                         " after receiving ICF");
                }

                if (mainPhyLinkId != linkId)
                {
                    CheckMainPhyTraceInfo(i, "DlTxopIcfReceivedByAuxPhy", mainPhyLinkId, linkId);
                }

                CheckAuxPhysSleepMode(m_staMacs[i], true);
            });

            break;
        }
    }
}

void
EmlsrDlTxopTest::CheckQosFrames(const WifiConstPsduMap& psduMap,
                                const WifiTxVector& txVector,
                                uint8_t linkId)
{
    if (m_nEmlsrStations != 2 || m_apMac->GetNLinks() != m_emlsrLinks.size() ||
        m_emlsrEnabledTime.IsZero() || Simulator::Now() < m_emlsrEnabledTime + m_fe2to3delay)

    {
        // we are interested in frames sent to test transition delay
        return;
    }

    std::size_t firstClientId = 0;
    std::size_t secondClientId = 1;
    auto addr = m_staMacs[secondClientId]->GetAddress();
    auto txDuration =
        WifiPhy::CalculateTxDuration(psduMap, txVector, m_apMac->GetWifiPhy(linkId)->GetPhyBand());

    m_countQoSframes++;

    switch (m_countQoSframes)
    {
    case 1:
        // generate another small packet addressed to the first EMLSR client only
        m_apMac->GetDevice()->GetNode()->AddApplication(
            GetApplication(DOWNLINK, firstClientId, 1, 40));
        // both EMLSR clients are about to receive a QoS data frame
        for (std::size_t clientId : {firstClientId, secondClientId})
        {
            Simulator::Schedule(txDuration, [=, this]() {
                for (uint8_t id = 0; id < m_staMacs[clientId]->GetNLinks(); id++)
                {
                    // link on which QoS data is received is not blocked
                    CheckBlockedLink(m_staMacs[clientId],
                                     m_apMac->GetAddress(),
                                     id,
                                     WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                     id != linkId,
                                     "Checking EMLSR links on EMLSR client " +
                                         std::to_string(clientId) +
                                         " after receiving the first QoS data frame");
                }
            });
        }
        break;
    case 2:
        // generate another small packet addressed to the second EMLSR client
        m_apMac->GetDevice()->GetNode()->AddApplication(
            GetApplication(DOWNLINK, secondClientId, 1, 40));

        // when the transmission of the second QoS data frame starts, both EMLSR clients are
        // still blocking all the links but the one used to receive the QoS data frame
        for (std::size_t clientId : {firstClientId, secondClientId})
        {
            for (uint8_t id = 0; id < m_staMacs[clientId]->GetNLinks(); id++)
            {
                // link on which QoS data is received is not blocked
                CheckBlockedLink(m_staMacs[clientId],
                                 m_apMac->GetAddress(),
                                 id,
                                 WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                 id != linkId,
                                 "Checking EMLSR links on EMLSR client " +
                                     std::to_string(clientId) +
                                     " when starting the reception of the second QoS frame");
            }
        }

        // the EMLSR client that is not the recipient of the QoS frame being transmitted will
        // switch back to listening mode after a transition delay starting from the end of
        // the PPDU carrying this QoS data frame

        // immediately before the end of the PPDU, this link only is not blocked for the EMLSR
        // client on the AP MLD
        Simulator::Schedule(txDuration - NanoSeconds(1), [=, this]() {
            for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
            {
                CheckBlockedLink(m_apMac,
                                 addr,
                                 id,
                                 WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                 id != linkId,
                                 "Checking that links of EMLSR client " +
                                     std::to_string(secondClientId) +
                                     " are blocked on the AP MLD before the end of the PPDU");
            }
        });
        // immediately before the end of the PPDU, all the links on the EMLSR client that is not
        // the recipient of the second QoS frame are unblocked (they are unblocked when the
        // PHY-RXSTART.indication is not received)
        Simulator::Schedule(txDuration - NanoSeconds(1), [=, this]() {
            for (uint8_t id = 0; id < m_staMacs[secondClientId]->GetNLinks(); id++)
            {
                CheckBlockedLink(m_staMacs[secondClientId],
                                 m_apMac->GetAddress(),
                                 id,
                                 WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                 false,
                                 "Checking that links of EMLSR client " +
                                     std::to_string(secondClientId) +
                                     " are unblocked before the end of the second QoS frame");
            }
        });
        // immediately after the end of the PPDU, all links are blocked for the EMLSR client
        Simulator::Schedule(txDuration + NanoSeconds(1), [=, this]() {
            for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
            {
                CheckBlockedLink(m_apMac,
                                 addr,
                                 id,
                                 WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                                 true,
                                 "Checking links of EMLSR client " +
                                     std::to_string(secondClientId) +
                                     " are all blocked on the AP MLD after the end of the PPDU");
            }
        });
        // immediately before the transition delay, all links are still blocked for the EMLSR client
        Simulator::Schedule(
            txDuration + m_transitionDelay.at(secondClientId) - NanoSeconds(1),
            [=, this]() {
                for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
                {
                    CheckBlockedLink(
                        m_apMac,
                        addr,
                        id,
                        WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                        true,
                        "Checking links of EMLSR client " + std::to_string(secondClientId) +
                            " are all blocked on the AP MLD before the transition delay",
                        false);
                }
            });

        // 100 us before the transition delay expires, generate another small packet addressed
        // to a non-EMLSR client. The AP will start a TXOP to transmit this frame, while the
        // frame addressed to the EMLSR client is still queued because the transition delay has
        // not yet elapsed. The transition delay will expire while the AP is transmitting the
        // frame to the non-EMLSR client, so that the AP continues the TXOP to transmit the frame
        // to the EMLSR client and we can check that the AP performs PIFS recovery after missing
        // the BlockAck from the EMLSR client
        Simulator::Schedule(txDuration + m_transitionDelay.at(secondClientId) - MicroSeconds(100),
                            [=, this]() {
                                m_apMac->GetDevice()->GetNode()->AddApplication(
                                    GetApplication(DOWNLINK, m_nEmlsrStations, 1, 40));
                            });

        break;
    case 3:
        // this is the frame addressed to a non-EMLSR client, which is transmitted before the
        // frame addressed to the EMLSR client, because the links of the latter are still blocked
        // at the AP because the transition delay has not yet elapsed
        NS_TEST_EXPECT_MSG_EQ(
            psduMap.cbegin()->second->GetAddr1(),
            m_staMacs[m_nEmlsrStations]->GetFrameExchangeManager(linkId)->GetAddress(),
            "QoS frame not addressed to a non-EMLSR client");

        for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
        {
            CheckBlockedLink(m_apMac,
                             addr,
                             id,
                             WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                             true,
                             "Checking links of EMLSR client " + std::to_string(secondClientId) +
                                 " are all blocked on the AP MLD before the transition delay");
        }
        // Block transmissions to the EMLSR client on all the links but the one on which this
        // frame is sent, so that the AP will continue this TXOP to send the queued frame to the
        // EMLSR client once the transition delay elapses
        for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
        {
            if (id != linkId)
            {
                m_apMac->BlockUnicastTxOnLinks(WifiQueueBlockedReason::TID_NOT_MAPPED, addr, {id});
            }
        }
        break;
    case 4:
        // the AP is continuing the TXOP, no need to block transmissions anymore
        for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
        {
            m_apMac->UnblockUnicastTxOnLinks(WifiQueueBlockedReason::TID_NOT_MAPPED, addr, {id});
        }
        // at the end of the fourth QoS frame, this link only is not blocked on the EMLSR
        // client receiving the frame
        Simulator::Schedule(txDuration, [=, this]() {
            for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
            {
                CheckBlockedLink(m_staMacs[secondClientId],
                                 m_apMac->GetAddress(),
                                 id,
                                 WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                 id != linkId,
                                 "Checking EMLSR links on EMLSR client " +
                                     std::to_string(secondClientId) +
                                     " after receiving the fourth QoS data frame");
            }
        });
        break;
    }
}

void
EmlsrDlTxopTest::CheckBlockAck(const WifiConstPsduMap& psduMap,
                               const WifiTxVector& txVector,
                               uint8_t phyId)
{
    if (m_nEmlsrStations != 2 || m_apMac->GetNLinks() != m_emlsrLinks.size() ||
        m_emlsrEnabledTime.IsZero() || Simulator::Now() < m_emlsrEnabledTime + m_fe2to3delay)
    {
        // we are interested in frames sent to test transition delay
        return;
    }

    if (++m_countBlockAck == 4)
    {
        // fourth BlockAck is sent by a non-EMLSR client
        return;
    }

    auto taddr = psduMap.cbegin()->second->GetAddr2();
    std::size_t clientId;
    if (m_staMacs[0]->GetLinkIdByAddress(taddr))
    {
        clientId = 0;
    }
    else
    {
        NS_TEST_ASSERT_MSG_EQ(m_staMacs[1]->GetLinkIdByAddress(taddr).has_value(),
                              true,
                              "Unexpected TA for BlockAck: " << taddr);
        clientId = 1;
    }

    // find the link on which the main PHY is operating
    auto currMainPhyLinkId = m_staMacs[clientId]->GetLinkForPhy(phyId);
    NS_TEST_ASSERT_MSG_EQ(
        currMainPhyLinkId.has_value(),
        true,
        "Didn't find the link on which the PHY sending the BlockAck is operating");
    auto linkId = *currMainPhyLinkId;

    // we need the MLD address to check the status of the container queues
    auto addr = m_apMac->GetWifiRemoteStationManager(linkId)->GetMldAddress(taddr);
    NS_TEST_ASSERT_MSG_EQ(addr.has_value(), true, "MLD address not found for " << taddr);

    auto apPhy = m_apMac->GetWifiPhy(linkId);
    auto txDuration = WifiPhy::CalculateTxDuration(psduMap, txVector, apPhy->GetPhyBand());
    auto cfEndTxDuration = WifiPhy::CalculateTxDuration(
        Create<WifiPsdu>(Create<Packet>(), WifiMacHeader(WIFI_MAC_CTL_END)),
        m_apMac->GetWifiRemoteStationManager(linkId)->GetRtsTxVector(Mac48Address::GetBroadcast(),
                                                                     txVector.GetChannelWidth()),
        apPhy->GetPhyBand());

    switch (m_countBlockAck)
    {
    case 5:
        // the PPDU carrying this BlockAck is corrupted, hence the AP MLD MAC receives the
        // PHY-RXSTART indication but it does not receive any frame from the PHY. Therefore,
        // at the end of the PPDU transmission, the AP MLD realizes that the EMLSR client has
        // not responded and makes an attempt at continuing the TXOP

        // at the end of the PPDU, this link only is not blocked on both the EMLSR client and
        // the AP MLD
        Simulator::Schedule(txDuration, [=, this]() {
            for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
            {
                CheckBlockedLink(m_staMacs[clientId],
                                 m_apMac->GetAddress(),
                                 id,
                                 WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                 id != linkId,
                                 "Checking links on EMLSR client " + std::to_string(clientId) +
                                     " at the end of fourth BlockAck");
                CheckBlockedLink(m_apMac,
                                 *addr,
                                 id,
                                 WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                 id != linkId,
                                 "Checking links of EMLSR client " + std::to_string(clientId) +
                                     " on the AP MLD at the end of fourth BlockAck");
            }
        });
        // a SIFS after the end of the PPDU, still this link only is not blocked on both the
        // EMLSR client and the AP MLD
        Simulator::Schedule(txDuration + apPhy->GetSifs(), [=, this]() {
            for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
            {
                CheckBlockedLink(m_staMacs[clientId],
                                 m_apMac->GetAddress(),
                                 id,
                                 WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                 id != linkId,
                                 "Checking links on EMLSR client " + std::to_string(clientId) +
                                     " a SIFS after the end of fourth BlockAck");
                CheckBlockedLink(m_apMac,
                                 *addr,
                                 id,
                                 WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                 id != linkId,
                                 "Checking links of EMLSR client " + std::to_string(clientId) +
                                     " a SIFS after the end of fourth BlockAck");
            }
        });
        // corrupt this BlockAck so that the AP MLD sends a BlockAckReq later on
        {
            auto uid = psduMap.cbegin()->second->GetPacket()->GetUid();
            m_errorModel->SetList({uid});
        }
        break;
    case 6:
        // at the end of the PPDU, this link only is not blocked on both the EMLSR client and
        // the AP MLD
        Simulator::Schedule(txDuration, [=, this]() {
            for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
            {
                CheckBlockedLink(m_staMacs[clientId],
                                 m_apMac->GetAddress(),
                                 id,
                                 WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                 id != linkId,
                                 "Checking links on EMLSR client " + std::to_string(clientId) +
                                     " at the end of fifth BlockAck");
                CheckBlockedLink(m_apMac,
                                 *addr,
                                 id,
                                 WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                 id != linkId,
                                 "Checking links of EMLSR client " + std::to_string(clientId) +
                                     " on the AP MLD at the end of fifth BlockAck");
            }
        });
        // before the end of the CF-End frame, still this link only is not blocked on both the
        // EMLSR client and the AP MLD
        Simulator::Schedule(
            txDuration + apPhy->GetSifs() + cfEndTxDuration - MicroSeconds(1),
            [=, this]() {
                for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
                {
                    CheckBlockedLink(m_staMacs[clientId],
                                     m_apMac->GetAddress(),
                                     id,
                                     WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                     id != linkId,
                                     "Checking links on EMLSR client " + std::to_string(clientId) +
                                         " before the end of CF-End frame");
                    CheckBlockedLink(m_apMac,
                                     *addr,
                                     id,
                                     WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                     id != linkId,
                                     "Checking links of EMLSR client " + std::to_string(clientId) +
                                         " on the AP MLD before the end of CF-End frame");
                }
            });
        // after the end of the CF-End frame, all links for the EMLSR client are blocked on the
        // AP MLD
        Simulator::Schedule(
            txDuration + apPhy->GetSifs() + cfEndTxDuration + MicroSeconds(1),
            [=, this]() {
                for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
                {
                    CheckBlockedLink(
                        m_apMac,
                        *addr,
                        id,
                        WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                        true,
                        "Checking links of EMLSR client " + std::to_string(clientId) +
                            " are all blocked on the AP MLD right after the end of CF-End");
                }
            });
        // before the end of the transition delay, all links for the EMLSR client are still
        // blocked on the AP MLD
        Simulator::Schedule(
            txDuration + apPhy->GetSifs() + cfEndTxDuration + m_transitionDelay.at(clientId) -
                MicroSeconds(1),
            [=, this]() {
                for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
                {
                    CheckBlockedLink(
                        m_apMac,
                        *addr,
                        id,
                        WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                        true,
                        "Checking links of EMLSR client " + std::to_string(clientId) +
                            " are all blocked on the AP MLD before the end of transition delay");
                }
            });
        // immediately after the transition delay, all links for the EMLSR client are unblocked
        Simulator::Schedule(
            txDuration + apPhy->GetSifs() + cfEndTxDuration + m_transitionDelay.at(clientId) +
                MicroSeconds(1),
            [=, this]() {
                for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
                {
                    CheckBlockedLink(
                        m_apMac,
                        *addr,
                        id,
                        WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                        false,
                        "Checking links of EMLSR client " + std::to_string(clientId) +
                            " are all unblocked on the AP MLD after the transition delay");
                }
            });
        break;
    }
}

void
EmlsrDlTxopTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

EmlsrUlTxopTest::EmlsrUlTxopTest(const Params& params)
    : EmlsrOperationsTestBase("Check EML UL TXOP transmissions (genBackoffAndUseAuxPhyCca=" +
                              std::to_string(params.genBackoffAndUseAuxPhyCca) +
                              ", nSlotsLeftAlert=" + std::to_string(params.nSlotsLeftAlert)),
      m_emlsrLinks(params.linksToEnableEmlsrOn),
      m_channelWidth(params.channelWidth),
      m_auxPhyChannelWidth(params.auxPhyChannelWidth),
      m_mediumSyncDuration(params.mediumSyncDuration),
      m_msdMaxNTxops(params.msdMaxNTxops),
      m_emlsrEnabledTime(0),
      m_firstUlPktsGenTime(0),
      m_unblockMainPhyLinkDelay(MilliSeconds(20)),
      m_checkBackoffStarted(false),
      m_countQoSframes(0),
      m_countBlockAck(0),
      m_countRtsframes(0),
      m_genBackoffIfTxopWithoutTx(params.genBackoffAndUseAuxPhyCca),
      m_useAuxPhyCca(params.genBackoffAndUseAuxPhyCca),
      m_nSlotsLeftAlert(params.nSlotsLeftAlert),
      m_switchMainPhyBackDelayTimeout(params.switchMainPhyBackDelayTimeout),
      m_5thQosFrameExpWidth(0)
{
    m_nEmlsrStations = 1;
    m_nNonEmlsrStations = 0;
    m_linksToEnableEmlsrOn = params.linksToEnableEmlsrOn;
    m_mainPhyId = 1;

    // when aux PHYs do not switch link, the main PHY switches back to its previous link after
    // a TXOP, hence the transition delay must exceed the channel switch delay (default: 250us)
    m_transitionDelay = {MicroSeconds(256)};
    m_establishBaDl = true;
    m_establishBaUl = true;
    m_putAuxPhyToSleep = params.putAuxPhyToSleep;
    m_duration = Seconds(1);

    NS_ABORT_MSG_IF(params.linksToEnableEmlsrOn.size() < 2,
                    "This test requires at least two links to be configured as EMLSR links");
    for (uint8_t id = 0; id < 3; id++)
    {
        if (!m_emlsrLinks.contains(id))
        {
            // non-EMLSR link found
            m_nonEmlsrLink = id;
            break;
        }
    }
}

void
EmlsrUlTxopTest::DoSetup()
{
    Config::SetDefault("ns3::EmlsrManager::AuxPhyChannelWidth",
                       UintegerValue(m_auxPhyChannelWidth));
    Config::SetDefault("ns3::DefaultEmlsrManager::SwitchAuxPhy", BooleanValue(false));
    Config::SetDefault("ns3::AdvancedEmlsrManager::UseAuxPhyCca", BooleanValue(m_useAuxPhyCca));
    Config::SetDefault("ns3::AdvancedEmlsrManager::SwitchMainPhyBackDelay",
                       TimeValue(MilliSeconds(m_switchMainPhyBackDelayTimeout ? 2 : 0)));
    Config::SetDefault("ns3::EhtConfiguration::MediumSyncDuration",
                       TimeValue(m_mediumSyncDuration));
    Config::SetDefault("ns3::EhtConfiguration::MsdMaxNTxops", UintegerValue(m_msdMaxNTxops));
    Config::SetDefault("ns3::ChannelAccessManager::GenerateBackoffIfTxopWithoutTx",
                       BooleanValue(m_genBackoffIfTxopWithoutTx));
    Config::SetDefault("ns3::ChannelAccessManager::NSlotsLeft", UintegerValue(m_nSlotsLeftAlert));
    // Channel switch delay should be less than RTS TX time + SIFS + CTS TX time, otherwise
    // UL TXOPs cannot be initiated by aux PHYs
    Config::SetDefault("ns3::WifiPhy::ChannelSwitchDelay", TimeValue(MicroSeconds(75)));
    Config::SetDefault("ns3::WifiPhy::NotifyMacHdrRxEnd", BooleanValue(true));

    EmlsrOperationsTestBase::DoSetup();

    m_staMacs[0]->GetQosTxop(AC_BE)->TraceConnectWithoutContext(
        "BackoffTrace",
        MakeCallback(&EmlsrUlTxopTest::BackoffGenerated, this));

    uint8_t linkId = 0;
    // configure channels of the given width
    for (auto band : {WIFI_PHY_BAND_2_4GHZ, WIFI_PHY_BAND_5GHZ, WIFI_PHY_BAND_6GHZ})
    {
        MHz_u bw{20};
        uint8_t number = band == WIFI_PHY_BAND_5GHZ ? 36 : 1;

        auto width =
            std::min(m_channelWidth, band == WIFI_PHY_BAND_2_4GHZ ? MHz_u{40} : MHz_u{160});
        while (bw < width)
        {
            bw *= 2;
            number += Count20MHzSubchannels(bw);
        }

        for (auto mac : std::initializer_list<Ptr<WifiMac>>{m_apMac, m_staMacs[0]})
        {
            mac->GetWifiPhy(linkId)->SetOperatingChannel(
                WifiPhy::ChannelTuple{number, width, band, 0});
        }
        linkId++;
    }

    // install post reception error model on the AP affiliated with the AP MLD and operating on
    // the same link as the main PHY of the EMLSR client
    m_errorModel = CreateObject<ListErrorModel>();
    m_apMac->GetWifiPhy(m_mainPhyId)->SetPostReceptionErrorModel(m_errorModel);
}

void
EmlsrUlTxopTest::BackoffGenerated(uint32_t backoff, uint8_t linkId)
{
    NS_LOG_INFO("Backoff value " << backoff << " generated by EMLSR client on link " << +linkId
                                 << "\n");
    if (linkId != m_mainPhyId)
    {
        return; // we are only interested in backoff on main PHY link
    }

    if (m_backoffEndTime)
    {
        if (m_checkBackoffStarted)
        {
            // another backoff value while checkBackoffStarted is true is generated only if
            // GenerateBackoffIfTxopWithoutTx is true
            NS_TEST_EXPECT_MSG_EQ(
                m_genBackoffIfTxopWithoutTx,
                true,
                "Another backoff value should not be generated while the main PHY link is blocked");

            NS_TEST_EXPECT_MSG_EQ(m_backoffEndTime.value(),
                                  Simulator::Now(),
                                  "Backoff generated at unexpected time");
        }
        else
        {
            // we are done checking the backoff
            m_backoffEndTime.reset();
        }
    }

    if (m_checkBackoffStarted)
    {
        if (!m_backoffEndTime.has_value())
        {
            // this is the first time we set m_backoffEndTime, which is done right after receiving
            // a BlockAck, thus we have to wait an AIFS before invoking backoff
            m_backoffEndTime = Simulator::Now() +
                               m_staMacs[0]->GetChannelAccessManager(linkId)->GetSifs() +
                               m_staMacs[0]->GetQosTxop(AC_BE)->GetAifsn(linkId) *
                                   m_staMacs[0]->GetChannelAccessManager(linkId)->GetSlot();
        }
        else
        {
            // we get here when the backoff expired but no transmission occurred, thus we have
            // generated a new backoff value and we will start decrementing the counter in a slot
            m_backoffEndTime =
                Simulator::Now() + m_staMacs[0]->GetChannelAccessManager(linkId)->GetSlot();
        }
        // add the time corresponding to the generated number of slots
        m_backoffEndTime.value() +=
            backoff * m_staMacs[0]->GetChannelAccessManager(linkId)->GetSlot();
        NS_LOG_DEBUG("Expected backoff end time = " << m_backoffEndTime->As(Time::US) << "\n");
    }
}

void
EmlsrUlTxopTest::Transmit(Ptr<WifiMac> mac,
                          uint8_t phyId,
                          WifiConstPsduMap psduMap,
                          WifiTxVector txVector,
                          double txPowerW)
{
    EmlsrOperationsTestBase::Transmit(mac, phyId, psduMap, txVector, txPowerW);
    auto linkId = m_txPsdus.back().linkId;

    auto psdu = psduMap.begin()->second;
    auto nodeId = mac->GetDevice()->GetNode()->GetId();

    switch (psdu->GetHeader(0).GetType())
    {
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
        NS_ASSERT_MSG(nodeId > 0, "APs do not send AssocReq frames");
        NS_TEST_EXPECT_MSG_EQ(+linkId, +m_mainPhyId, "AssocReq not sent by the main PHY");
        break;

    case WIFI_MAC_CTL_RTS:
        CheckRtsFrames(*psdu->begin(), txVector, linkId);
        break;

    case WIFI_MAC_CTL_CTS:
        CheckCtsFrames(*psdu->begin(), txVector, linkId);
        break;

    case WIFI_MAC_QOSDATA:
        CheckQosFrames(psduMap, txVector, linkId);
        break;

    case WIFI_MAC_CTL_BACKRESP:
        CheckBlockAck(psduMap, txVector, linkId);
        break;

    default:;
    }
}

void
EmlsrUlTxopTest::StartTraffic()
{
    // initially, we prevent transmissions on aux PHY links
    auto auxPhyLinks = m_staMacs[0]->GetSetupLinkIds();
    auxPhyLinks.erase(m_mainPhyId);
    if (m_nonEmlsrLink)
    {
        auxPhyLinks.erase(*m_nonEmlsrLink);
    }
    m_staMacs[0]->BlockUnicastTxOnLinks(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                        m_apMac->GetAddress(),
                                        auxPhyLinks);

    // Association, Block Ack agreement establishment and enabling EMLSR mode have been done.
    // After 50ms, schedule:
    // - block of transmissions on the link where the main PHY is operating and on the non-EMLSR
    //   link (if any)
    // - the generation of two UL packets
    // - after m_unblockMainPhyLinkDelay, unblock transmissions on the link where the main PHY
    //   is operating, so that the first data frame is transmitted on that link
    Simulator::Schedule(MilliSeconds(50), [&]() {
        std::set<uint8_t> linkIds;
        linkIds.insert(*m_staMacs[0]->GetLinkForPhy(m_mainPhyId));
        if (m_nonEmlsrLink)
        {
            linkIds.insert(*m_nonEmlsrLink);
        }
        m_staMacs[0]->BlockUnicastTxOnLinks(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                            m_apMac->GetAddress(),
                                            linkIds);

        NS_LOG_INFO("Enqueuing two packets at the EMLSR client\n");
        m_staMacs[0]->GetDevice()->GetNode()->AddApplication(GetApplication(UPLINK, 0, 2, 1000));
        m_firstUlPktsGenTime = Simulator::Now();

        Simulator::Schedule(m_unblockMainPhyLinkDelay, [&]() {
            m_staMacs[0]->UnblockUnicastTxOnLinks(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                  m_apMac->GetAddress(),
                                                  {*m_staMacs[0]->GetLinkForPhy(m_mainPhyId)});
        });
    });
}

void
EmlsrUlTxopTest::CheckQosFrames(const WifiConstPsduMap& psduMap,
                                const WifiTxVector& txVector,
                                uint8_t linkId)
{
    m_countQoSframes++;

    auto txDuration =
        WifiPhy::CalculateTxDuration(psduMap, txVector, m_apMac->GetWifiPhy(linkId)->GetPhyBand());

    switch (m_countQoSframes)
    {
    case 1:
    case 2:
        // do nothing, these are the QoS data frames sent to establish BA agreements in DL and UL
        // direction
        break;
    case 3:
        // first UL data frame (transmitted by the main PHY)
        if (m_nonEmlsrLink)
        {
            // generate data packets for another UL data frame, which will be sent on the
            // non-EMLSR link
            NS_LOG_INFO("Enqueuing two packets at the EMLSR client\n");
            m_staMacs[0]->GetDevice()->GetNode()->AddApplication(
                GetApplication(UPLINK, 0, 2, 1000));

            // unblock transmissions on the non-EMLSR link once the two packets are queued
            Simulator::ScheduleNow([=, this]() {
                m_staMacs[0]->UnblockUnicastTxOnLinks(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                      m_apMac->GetAddress(),
                                                      {*m_nonEmlsrLink});
            });
        }

        // check that other EMLSR links are now blocked on the EMLSR client and on the AP MLD
        // after this QoS data frame is received
        Simulator::ScheduleNow([=, this]() {
            auto phyHdrTxTime = WifiPhy::CalculatePhyPreambleAndHeaderDuration(txVector);
            auto macHdrSize = (*psduMap.at(SU_STA_ID)->begin())->GetHeader().GetSerializedSize() +
                              4 /* A-MPDU subframe header size */;
            auto macHdrTxTime =
                DataRate(txVector.GetMode().GetDataRate(txVector)).CalculateBytesTxTime(macHdrSize);

            for (auto id : m_staMacs[0]->GetLinkIds())
            {
                CheckBlockedLink(
                    m_staMacs[0],
                    m_apMac->GetAddress(),
                    id,
                    WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                    id != m_staMacs[0]->GetLinkForPhy(m_mainPhyId) && m_staMacs[0]->IsEmlsrLink(id),
                    "Checking EMLSR links on EMLSR client while sending the first data frame",
                    false);

                Simulator::Schedule(phyHdrTxTime + macHdrTxTime + MicroSeconds(1), [=, this]() {
                    CheckBlockedLink(m_apMac,
                                     m_staMacs[0]->GetAddress(),
                                     id,
                                     WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                     id != m_staMacs[0]->GetLinkForPhy(m_mainPhyId) &&
                                         m_staMacs[0]->IsEmlsrLink(id),
                                     "Checking EMLSR links on AP MLD right after receiving the MAC "
                                     "header of the first data frame");
                });

                Simulator::Schedule(
                    txDuration + MicroSeconds(MAX_PROPAGATION_DELAY_USEC),
                    [=, this]() {
                        CheckBlockedLink(
                            m_apMac,
                            m_staMacs[0]->GetAddress(),
                            id,
                            WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                            id != m_staMacs[0]->GetLinkForPhy(m_mainPhyId) &&
                                m_staMacs[0]->IsEmlsrLink(id),
                            "Checking EMLSR links on AP MLD after sending the first data frame");
                    });
            }
        });

        if (m_nonEmlsrLink)
        {
            break;
        }
        m_countQoSframes++; // if all EMLSR links, next case is already executed now
        [[fallthrough]];
    case 4:
        // check that other EMLSR links are now blocked on the EMLSR client and on the AP MLD
        // after this QoS data frame is received
        Simulator::ScheduleNow([=, this]() {
            // make aux PHYs capable of transmitting frames
            auto auxPhyLinks = m_staMacs[0]->GetSetupLinkIds();
            auxPhyLinks.erase(m_mainPhyId);
            if (m_nonEmlsrLink)
            {
                auxPhyLinks.erase(*m_nonEmlsrLink);
            }
            m_staMacs[0]->UnblockUnicastTxOnLinks(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                  m_apMac->GetAddress(),
                                                  auxPhyLinks);

            // block transmissions on the link where the main PHY is operating
            m_staMacs[0]->BlockUnicastTxOnLinks(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                m_apMac->GetAddress(),
                                                {*m_staMacs[0]->GetLinkForPhy(m_mainPhyId)});

            // generate data packets for another UL data frame, which will be sent on a link on
            // which an aux PHY is operating
            NS_LOG_INFO("Enqueuing two packets at the EMLSR client\n");
            m_staMacs[0]->GetDevice()->GetNode()->AddApplication(
                GetApplication(UPLINK, 0, 2, 1000));
        });
        break;
    case 5:
        // check that other EMLSR links are now blocked on both the EMLSR client and the AP MLD
        Simulator::ScheduleNow([=, this]() {
            for (auto id : m_staMacs[0]->GetLinkIds())
            {
                CheckBlockedLink(
                    m_staMacs[0],
                    m_apMac->GetAddress(),
                    id,
                    WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                    id != linkId && m_staMacs[0]->IsEmlsrLink(id),
                    "Checking EMLSR links on EMLSR client while sending the second data frame",
                    false);

                CheckBlockedLink(
                    m_apMac,
                    m_staMacs[0]->GetAddress(),
                    id,
                    WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                    id != linkId && m_staMacs[0]->IsEmlsrLink(id),
                    "Checking EMLSR links on AP MLD while sending the second data frame",
                    false);
            }

            // unblock transmission on the link where the main PHY is operating
            m_staMacs[0]->GetMacQueueScheduler()->UnblockQueues(
                WifiQueueBlockedReason::TID_NOT_MAPPED,
                AC_BE,
                {WIFI_QOSDATA_QUEUE},
                m_apMac->GetAddress(),
                m_staMacs[0]->GetAddress(),
                {0},
                {m_mainPhyId});
        });
        break;
    }
}

void
EmlsrUlTxopTest::CheckBlockAck(const WifiConstPsduMap& psduMap,
                               const WifiTxVector& txVector,
                               uint8_t linkId)
{
    m_countBlockAck++;

    auto auxPhyLinks = m_staMacs[0]->GetSetupLinkIds();
    auxPhyLinks.erase(m_mainPhyId);
    if (m_nonEmlsrLink)
    {
        auxPhyLinks.erase(*m_nonEmlsrLink);
    }

    auto txDuration =
        WifiPhy::CalculateTxDuration(psduMap, txVector, m_apMac->GetWifiPhy(linkId)->GetPhyBand());

    // in this test, BlockAck frames terminates TXOP, thus aux PHYs shall be in sleep mode before
    // the end of BlockAck reception and awake right afterwards
    if (linkId != m_nonEmlsrLink)
    {
        Simulator::Schedule(txDuration - TimeStep(1),
                            &EmlsrUlTxopTest::CheckAuxPhysSleepMode,
                            this,
                            m_staMacs[0],
                            true);
        Simulator::Schedule(txDuration + TimeStep(1),
                            &EmlsrUlTxopTest::CheckAuxPhysSleepMode,
                            this,
                            m_staMacs[0],
                            false);

        // if the TXOP has been carried out on a link other than the preferred link, the main PHY
        // switches back to the preferred link when the TXOP ends
        if (m_staMacs[0]->GetLinkForPhy(m_mainPhyId) != linkId)
        {
            Simulator::Schedule(txDuration + TimeStep(1), [=, this]() {
                // check the traced remaining time before calling CheckMainPhyTraceInfo
                if (const auto traceInfoIt = m_traceInfo.find(0);
                    traceInfoIt != m_traceInfo.cend() &&
                    traceInfoIt->second->GetName() == "TxopEnded")
                {
                    const auto& traceInfo =
                        static_cast<const EmlsrTxopEndedTrace&>(*traceInfoIt->second);
                    NS_TEST_EXPECT_MSG_EQ(
                        traceInfo.remTime,
                        Time{0},
                        "Expected null remaining time because TXOP ended regularly");
                }

                CheckMainPhyTraceInfo(0, "TxopEnded", linkId, m_mainPhyId);
            });
        }
    }

    switch (m_countBlockAck)
    {
    case 1:
    case 2:
        // do nothing, these are BlockAcks in response to the QoS data frames sent to establish
        // BA agreements in DL and UL direction
        break;
    case 3:
        if (linkId == m_nonEmlsrLink)
        {
            // this BlockAck has been sent on the non-EMLSR link, ignore it
            break;
        }
        m_checkBackoffStarted = true;
        if (!m_nonEmlsrLink)
        {
            m_countBlockAck++; // if all EMLSR links, next case is already executed now
        }
        [[fallthrough]];
    case 4:
        if (m_nonEmlsrLink && m_countBlockAck == 4)
        {
            // block transmissions on the non-EMLSR link
            Simulator::Schedule(txDuration + NanoSeconds(1), [=, this]() {
                m_staMacs[0]->BlockUnicastTxOnLinks(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                    m_apMac->GetAddress(),
                                                    {*m_nonEmlsrLink});
            });
        }
        if (linkId == m_nonEmlsrLink)
        {
            // this BlockAck has been sent on the non-EMLSR link, ignore it
            break;
        }
        m_checkBackoffStarted = true;
        break;
    case 5:
        // Block Ack in response to the second data frame sent by the EMLSR client on EMLSR links.
        // Check that MediumSyncDelay timer starts running on the link where the main PHY switches
        // to when the channel switch is completed
        Simulator::Schedule(
            txDuration + m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId)->GetChannelSwitchDelay() +
                NanoSeconds(1),
            [=, this]() {
                auto elapsed =
                    m_staMacs[0]->GetEmlsrManager()->GetElapsedMediumSyncDelayTimer(m_mainPhyId);
                NS_TEST_EXPECT_MSG_EQ(
                    elapsed.has_value(),
                    true,
                    "MediumSyncDelay timer not running on link where main PHY is operating");
                m_lastMsdExpiryTime = Simulator::Now() +
                                      m_staMacs[0]->GetEmlsrManager()->GetMediumSyncDuration() -
                                      *elapsed;
            });

        // Check that the number of backoff slots is not changed since the beginning of the TXOP
        Simulator::Schedule(txDuration, [=, this]() {
            m_checkBackoffStarted = false;
            NS_TEST_ASSERT_MSG_EQ(m_backoffEndTime.has_value(),
                                  true,
                                  "Backoff end time should have been calculated");
            // when this BlockAck is received, the TXOP ends and the main PHY link is unblocked,
            // which causes a new backoff timer to be generated if the backoff timer is not
            // already running
            *m_backoffEndTime = Max(*m_backoffEndTime, Simulator::Now());
        });

        // make aux PHYs not capable of transmitting frames
        m_staMacs[0]->BlockUnicastTxOnLinks(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                            m_apMac->GetAddress(),
                                            auxPhyLinks);

        // generate data packets for another UL data frame, which will be sent on the link where
        // the main PHY is operating
        NS_LOG_INFO("Enqueuing two packets at the EMLSR client\n");
        m_staMacs[0]->GetDevice()->GetNode()->AddApplication(GetApplication(UPLINK, 0, 2, 1000));
        break;
    case 6: {
        // block transmission on the main PHY link and on the non-EMLSR link (if any), so that
        // the next QoS frames are sent on a link where an aux PHY is operating
        std::set<uint8_t> linkIds{m_mainPhyId};
        if (m_nonEmlsrLink)
        {
            linkIds.insert(*m_nonEmlsrLink);
        }
        m_staMacs[0]->GetMacQueueScheduler()->BlockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                          AC_BE,
                                                          {WIFI_QOSDATA_QUEUE},
                                                          m_apMac->GetAddress(),
                                                          m_staMacs[0]->GetAddress(),
                                                          {0},
                                                          linkIds);
    }
        // make sure aux PHYs are capable of transmitting frames
        m_staMacs[0]->UnblockUnicastTxOnLinks(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                              m_apMac->GetAddress(),
                                              auxPhyLinks);

        // generate data packets for another UL data frame
        NS_LOG_INFO("Enqueuing two packets at the EMLSR client\n");
        m_staMacs[0]->GetDevice()->GetNode()->AddApplication(GetApplication(UPLINK, 0, 2, 1000));
        break;
    case 7:
        // make the aux PHY(s) not capable of transmitting frames
        m_staMacs[0]->GetEmlsrManager()->SetAuxPhyTxCapable(false);
        if (!m_nonEmlsrLink)
        {
            // if there are two auxiliary links, set MediumSyncDuration to zero so that the
            // next UL QoS data frame is not protected also in case it is transmitted on the
            // auxiliary link other than the one on which the last frame exchange occurred
            m_staMacs[0]->GetEmlsrManager()->SetMediumSyncDuration(Seconds(0));
        }

        // generate a very large backoff for the preferred link, so that when an aux PHY gains a
        // TXOP, it requests the main PHY to switch to its link to transmit the frames
        m_staMacs[0]->GetQosTxop(AC_BE)->StartBackoffNow(100, m_mainPhyId);

        // events to be scheduled at the end of the BlockAck response
        Simulator::Schedule(txDuration + NanoSeconds(1), [=, this]() {
            // check that the main PHY switches to its preferred link
            auto mainPhy = m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId);

            NS_TEST_EXPECT_MSG_EQ(mainPhy->IsStateSwitching(),
                                  true,
                                  "Main PHY is not switching at time "
                                      << Simulator::Now().As(Time::NS));

            // events to be scheduled when the first main PHY channel switch is completed
            Simulator::Schedule(mainPhy->GetChannelSwitchDelay(), [=, this]() {
                // either the main PHY is operating on the preferred link or it is switching again
                auto mainPhyLinkid = m_staMacs[0]->GetLinkForPhy(mainPhy);
                if (mainPhyLinkid)
                {
                    NS_TEST_EXPECT_MSG_EQ(+mainPhyLinkid.value(),
                                          +m_mainPhyId,
                                          "Main PHY expected to operate on the preferred link");
                }
                else
                {
                    NS_TEST_EXPECT_MSG_EQ(
                        mainPhy->IsStateSwitching(),
                        true,
                        "Main PHY is not operating on a link and it is not switching at time "
                            << Simulator::Now().As(Time::NS));
                }

                auto acBe = m_staMacs[0]->GetQosTxop(AC_BE);

                // find the min remaining backoff time on auxiliary links for AC BE
                auto minBackoff = Time::Max();
                Time slot{0};
                for (uint8_t id = 0; id < m_staMacs[0]->GetNLinks(); id++)
                {
                    if (!m_staMacs[0]->GetWifiPhy(id))
                    {
                        continue; // no PHY on this link
                    }

                    if (auto backoff =
                            m_staMacs[0]->GetChannelAccessManager(id)->GetBackoffEndFor(acBe);
                        id != m_mainPhyId && m_staMacs[0]->IsEmlsrLink(id) && backoff < minBackoff)
                    {
                        minBackoff = backoff;
                        slot = m_staMacs[0]->GetWifiPhy(id)->GetSlot();
                    }
                }

                // if the backoff on a link has expired before the end of the main PHY channel
                // switch, the main PHY will be requested to switch again no later than the first
                // slot boundary after the end of the channel switch. Otherwise, it will be
                // requested to switch when the backoff expires or when the backoff counter reaches
                // the configured number of slots
                auto expected2ndSwitchDelay =
                    (minBackoff <= Simulator::Now()) ? mainPhy->GetSlot()
                    : m_nSlotsLeftAlert > 0
                        ? Max(minBackoff - m_nSlotsLeftAlert * slot - Simulator::Now(), Time{0})
                        : (minBackoff - Simulator::Now());

                // check that the main PHY is requested to switch to an auxiliary link after
                // the expected delay
                Simulator::Schedule(expected2ndSwitchDelay + NanoSeconds(1), [=, this]() {
                    NS_TEST_EXPECT_MSG_EQ(mainPhy->IsStateSwitching(),
                                          true,
                                          "Main PHY is not switching at time "
                                              << Simulator::Now().As(Time::NS));
                    NS_TEST_EXPECT_MSG_EQ(m_staMacs[0]->GetLinkForPhy(mainPhy).has_value(),
                                          false,
                                          "Main PHY should not be operating on a link because it "
                                          "should be switching to an auxiliary link");
                    // check that the appropriate trace info was received
                    CheckMainPhyTraceInfo(0,
                                          "UlTxopAuxPhyNotTxCapable",
                                          std::nullopt,
                                          0,
                                          false,
                                          false);

                    const auto delayUntilIdle = mainPhy->GetDelayUntilIdle();
                    auto startTimerDelay = delayUntilIdle;

                    if (m_switchMainPhyBackDelayTimeout)
                    {
                        TimeValue switchMainPhyBackDelay;
                        m_staMacs[0]->GetEmlsrManager()->GetAttribute("SwitchMainPhyBackDelay",
                                                                      switchMainPhyBackDelay);

                        // If nSlotsAlert is 0, the decision whether to start the switch back timer
                        // is taken at the end of the PIFS period during which we perform CCA and
                        // NAV check, which coincides with the end of the channel switch or is a
                        // PIFS afterwards, depending on whether aux PHY CCA is used. Therefore,
                        // before the end of the CCA and NAV check period we have to make the medium
                        // busy on the link the main PHY is switching to. Given that we do not know
                        // which link it is, we set the NAV on all links.
                        // If nSlotsAlert > 0, the decision whether to start the switch back timer
                        // is taken at the end of the channel switch and it is needed that the time
                        // until the backoff end is at least a PIFS to start the switch back timer.
                        auto endCcaNavCheckDelay = delayUntilIdle;

                        for (uint8_t id = 0; id < m_staMacs[0]->GetNLinks(); ++id)
                        {
                            if (auto phy = m_staMacs[0]->GetWifiPhy(id))
                            {
                                if (!m_useAuxPhyCca && m_nSlotsLeftAlert == 0)
                                {
                                    endCcaNavCheckDelay =
                                        Max(endCcaNavCheckDelay, delayUntilIdle + phy->GetPifs());
                                }

                                m_staMacs[0]->GetChannelAccessManager(id)->NotifyNavStartNow(
                                    endCcaNavCheckDelay + TimeStep(1));
                            }
                        }
                        startTimerDelay = endCcaNavCheckDelay;

                        // when the SwitchMainPhyBackDelay timer starts, extend the NAV on the
                        // aux PHY link on which the main PHY is operating by the timer duration
                        // plus a channel switch delay, so that the timer expires and the main PHY
                        // returns to the preferred link. If nSlotsAlert > 0, the timer duration is
                        // extended by the expected channel access when the main PHY switch ends.
                        Simulator::Schedule(startTimerDelay, [=, this]() {
                            auto auxLinkId = m_staMacs[0]->GetLinkForPhy(mainPhy);
                            NS_TEST_ASSERT_MSG_EQ(
                                auxLinkId.has_value(),
                                true,
                                "Main PHY should be operating on a link before timer expires");
                            auto timerDuration = switchMainPhyBackDelay.Get();
                            if (m_nSlotsLeftAlert > 0)
                            {
                                timerDuration += (m_staMacs[0]
                                                      ->GetChannelAccessManager(*auxLinkId)
                                                      ->GetBackoffEndFor(acBe) -
                                                  Simulator::Now());
                            }
                            m_staMacs[0]
                                ->GetChannelAccessManager(*auxLinkId)
                                ->NotifyNavStartNow(timerDuration +
                                                    mainPhy->GetChannelSwitchDelay());

                            // check that the SwitchMainPhyBackDelay timer expires and the main PHY
                            // returns to the preferred link
                            Simulator::Schedule(timerDuration + TimeStep(1), [=, this]() {
                                CheckMainPhyTraceInfo(0,
                                                      "TxopNotGainedOnAuxPhyLink",
                                                      std::nullopt,
                                                      m_mainPhyId,
                                                      false);
                            });
                        });
                    }

                    // events to be scheduled when main PHY finishes switching to auxiliary link
                    Simulator::Schedule(mainPhy->GetDelayUntilIdle(), [=, this]() {
                        auto auxLinkId = m_staMacs[0]->GetLinkForPhy(mainPhy);
                        NS_TEST_ASSERT_MSG_EQ(auxLinkId.has_value(),
                                              true,
                                              "Main PHY should have completed switching");
                        // update backoff on the auxiliary link on which main PHY is operating
                        auto cam = m_staMacs[0]->GetChannelAccessManager(*auxLinkId);
                        cam->NeedBackoffUponAccess(acBe, true, true);
                        const auto usedAuxPhyCca =
                            (m_useAuxPhyCca || m_auxPhyChannelWidth >= m_channelWidth) &&
                            (m_nSlotsLeftAlert == 0 ||
                             cam->GetBackoffEndFor(acBe) <= Simulator::Now());
                        m_5thQosFrameExpWidth =
                            usedAuxPhyCca ? m_auxPhyChannelWidth : m_channelWidth;
                        // record the time the transmission of the QoS data frames must have
                        // started: (a PIFS after) end of channel switch, if the backoff counter
                        // on the auxiliary link is null and UseAuxPhyCca is true (false); when
                        // the backoff expires, otherwise
                        if (auto slots = acBe->GetBackoffSlots(*auxLinkId); slots == 0)
                        {
                            m_5thQosFrameTxTime =
                                Simulator::Now() + (m_useAuxPhyCca ? Time{0} : mainPhy->GetPifs());
                        }
                        else
                        {
                            m_5thQosFrameTxTime = cam->GetBackoffEndFor(acBe);
                        }
                    });
                });
            });
        });

        // generate data packets for another UL data frame
        NS_LOG_INFO("Enqueuing two packets at the EMLSR client\n");
        m_staMacs[0]->GetDevice()->GetNode()->AddApplication(GetApplication(UPLINK, 0, 2, 1000));
        break;
    }
}

void
EmlsrUlTxopTest::CheckRtsFrames(Ptr<const WifiMpdu> mpdu,
                                const WifiTxVector& txVector,
                                uint8_t linkId)
{
    if (m_firstUlPktsGenTime.IsZero())
    {
        // this function only considers RTS frames sent after the first QoS data frame
        return;
    }

    if (linkId != m_mainPhyId)
    {
        if (m_countRtsframes > 0 && !m_corruptCts.has_value())
        {
            // we get here for the frame exchange in which the CTS response must be corrupted.
            // Install post reception error model on the STA affiliated with the EMLSR client that
            // is transmitting this RTS frame
            m_errorModel = CreateObject<ListErrorModel>();
            m_staMacs[0]->GetWifiPhy(linkId)->SetPostReceptionErrorModel(m_errorModel);
            m_corruptCts = true;
        }

        return;
    }

    // we get here for RTS frames sent by the main PHY while the MediumSyncDelay timer is running
    m_countRtsframes++;

    NS_TEST_EXPECT_MSG_EQ(txVector.GetChannelWidth(),
                          m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId)->GetChannelWidth(),
                          "RTS sent by main PHY on an unexpected width");

    // corrupt reception at AP MLD
    NS_LOG_INFO("CORRUPTED");
    m_errorModel->SetList({mpdu->GetPacket()->GetUid()});
}

void
EmlsrUlTxopTest::CheckCtsFrames(Ptr<const WifiMpdu> mpdu,
                                const WifiTxVector& txVector,
                                uint8_t linkId)
{
    if (m_firstUlPktsGenTime.IsZero())
    {
        // this function only considers CTS frames sent after the first QoS data frame
        return;
    }

    auto txDuration = WifiPhy::CalculateTxDuration(mpdu->GetSize(),
                                                   txVector,
                                                   m_apMac->GetWifiPhy(linkId)->GetPhyBand());
    const auto doCorruptCts = m_corruptCts.has_value() && *m_corruptCts;

    if (linkId != m_staMacs[0]->GetLinkForPhy(m_mainPhyId) && linkId != m_nonEmlsrLink &&
        mpdu->GetHeader().GetAddr1() == m_staMacs[0]->GetFrameExchangeManager(linkId)->GetAddress())
    {
        // this is a CTS sent to an aux PHY starting an UL TXOP. Given that aux PHYs do not
        // switch channel, they are put in sleep mode when the main PHY starts operating on their
        // link, which coincides with the end of CTS plus two propagation delays
        const auto auxPhy = m_staMacs[0]->GetWifiPhy(linkId);
        const auto mainPhy = m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId);
        Simulator::Schedule(txDuration, [=, this]() {
            // when CTS ends, the main PHY is still switching and the aux PHY is not yet sleeping
            NS_TEST_EXPECT_MSG_EQ(mainPhy->IsStateSwitching(),
                                  true,
                                  "Expecting the main PHY to be switching link");
            NS_TEST_EXPECT_MSG_EQ(auxPhy->IsStateSleep(),
                                  false,
                                  "Aux PHY on link " << +linkId << " already in sleep mode");
            // when CTS is sent, the main PHY may have already started switching, thus we may not
            // know which link the main PHY is moving from
            CheckMainPhyTraceInfo(0, "UlTxopRtsSentByAuxPhy", std::nullopt, linkId, false);
        });
        Simulator::Schedule(
            txDuration + MicroSeconds(2 * MAX_PROPAGATION_DELAY_USEC) + TimeStep(1),
            [=, this]() {
                // aux PHYs are put to sleep if and only if CTS is not corrupted
                // (causing the end of the TXOP)
                CheckAuxPhysSleepMode(m_staMacs[0], !doCorruptCts);
                // if CTS is corrupted, TXOP ends and the main PHY switches back
                // to the preferred link
                if (doCorruptCts)
                {
                    // check the traced remaining time before calling CheckMainPhyTraceInfo
                    if (const auto traceInfoIt = m_traceInfo.find(0);
                        traceInfoIt != m_traceInfo.cend() &&
                        traceInfoIt->second->GetName() == "TxopEnded")
                    {
                        const auto& traceInfo =
                            static_cast<const EmlsrTxopEndedTrace&>(*traceInfoIt->second);
                        NS_TEST_EXPECT_MSG_GT(traceInfo.remTime,
                                              Time{0},
                                              "Expected non-zero remaining time because main PHY "
                                              "was switching when TXOP ended");
                    }

                    CheckMainPhyTraceInfo(0, "TxopEnded", linkId, m_mainPhyId);
                }
            });
    }

    if (doCorruptCts)
    {
        // corrupt reception at EMLSR client
        NS_LOG_INFO("CORRUPTED");
        m_errorModel->SetList({mpdu->GetPacket()->GetUid()});
        m_corruptCts = false;
    }
}

void
EmlsrUlTxopTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

void
EmlsrUlTxopTest::CheckResults()
{
    if (m_msdMaxNTxops > 0)
    {
        NS_TEST_EXPECT_MSG_LT_OR_EQ(
            m_countRtsframes,
            +m_msdMaxNTxops,
            "Unexpected number of RTS frames sent while the MediumSyncDelay timer is running");
    }

    auto psduIt = m_txPsdus.cbegin();

    // lambda to jump to the next QoS data frame or MU-RTS Trigger Frame or RTS transmitted
    // to/by an EMLSR client
    auto jumpToQosDataOrMuRts = [&]() {
        while (psduIt != m_txPsdus.cend() &&
               !psduIt->psduMap.cbegin()->second->GetHeader(0).IsQosData() &&
               !psduIt->psduMap.cbegin()->second->GetHeader(0).IsRts())
        {
            auto psdu = psduIt->psduMap.cbegin()->second;
            if (psdu->GetHeader(0).IsTrigger())
            {
                CtrlTriggerHeader trigger;
                psdu->GetPayload(0)->PeekHeader(trigger);
                if (trigger.IsMuRts())
                {
                    break;
                }
            }
            psduIt++;
        }
    };

    /**
     * EMLSR client with EMLSR mode enabled on all links (main PHY ID = 1).
     *
     *  main PHY│
     *  blocked,│
     *  aux PHYs││main PHY blocked│
     *  cannot  │
     *  transmit│
     *          │                    ┌───┐         ┌──┐
     *  [link 0]                     │CTS│         │BA│
     *  ────────────────────────┬───┬┴───┴┬───┬───┬┴──┴─────────────────────────────────────────
     *                          │RTS│     │QoS│QoS│
     *                          └───┘     │ 6 │ 7 │
     *                                    └───┴───┘
     *                             gen backoff      gen backoff if     MediumSyncDelay
     *                    ┌──┐    (also many times)  not running       timer expired ┌──┐
     *  [link 1]          │BA│  │   if allowed        │                   │          │BA│
     *  ─────────┬───┬───┬┴──┴───────────────────────────┬───┬─────┬───┬────┬───┬───┬┴──┴───────
     *           │QoS│QoS│                               │RTS│ ... │RTS│    │QoS│QoS│
     *           │ 4 │ 5 │                               └───┘     └───┘    │ 8 │ 9 │
     *           └───┴───┘                                                  └───┴───┘
     *
     *  [link 2]
     *  ───────────────────────────────────────────────────────────────────────────
     *
     *
     *
     * EMLSR client with EMLSR mode enabled on links 0 and 1 (main PHY ID = 1).
     *
     * main PHY │
     *   and    │
     * non-EMLSR│
     *   link   │
     *  blocked,│
     *  aux PHYs││main PHY blocked│
     *  cannot  │
     *  transmit│
     *          │                    ┌───┐         ┌──┐
     *  [link 0]                     │CTS│         │BA│
     *  ────────────────────────┬───┬┴───┴┬───┬───┬┴──┴─────────────────────────────────────────
     *                          │RTS│     │QoS│QoS│
     *                          └───┘     │ 8 │ 9 │
     *                                    └───┴───┘
     *                             gen backoff      gen backoff if     MediumSyncDelay
     *                    ┌──┐    (also many times)  not running       timer expired ┌──┐
     *  [link 1]          │BA│  │   if allowed        │                   │          │BA│
     *  ─────────┬───┬───┬┴──┴───────────────────────────┬───┬─────┬───┬────┬───┬───┬┴──┴───────
     *           │QoS│QoS│                               │RTS│ ... │RTS│    │QoS│QoS│
     *           │ 4 │ 5 │                               └───┘     └───┘    │ 10│ 11│
     *           └───┴───┘                                                  └───┴───┘
     *                     ┌──┐
     *  [link 2]           │BA│
     *  ──────────┬───┬───┬┴──┴────────────────────────────────────────────────────────────
     *            │QoS│QoS│
     *            │ 6 │ 7 │
     *            └───┴───┘
     *
     * For both scenarios, after the last frame exchange on the main PHY link, we have the
     * following frame exchanges on an EMLSR link where an aux PHY is operating on. After the
     * first frame exchange, aux PHYs are configured as non-TX capable. Note that the two frame
     * exchanges may occur on distinct auxiliary EMLSR links.
     *
     *                                             | main PHY  || main PHY  |
     *  [ link ]   ┌───┐         ┌───┐         ┌──┐|switches to||switches to|             ┌──┐
     *  [0 or 2]   │CTS│         │CTS│         │BA│| preferred ||auxiliary  |PIFS|        │BA│
     *  ──────┬───┬┴───X────┬───┬┴───┴┬───┬───┬┴──┴──────────────────────────────┬───┬───┬┴──┴───
     *        │RTS│         │RTS│     │QoS│QoS│                                  │QoS│QoS│
     *        └───┘         └───┘     │ X │ Y │                                  │ Z │ W │
     *                                └───┴───┘                                  └───┴───┘
     * For all EMLSR links scenario, X=10, Y=11, Z=12, W=13
     * For the scenario with a non-EMLSR link, X=12, Y=13, Z=14, W=15
     */

    // jump to the first (non-Beacon) frame transmitted after establishing BA agreements and
    // enabling EMLSR mode
    while (psduIt != m_txPsdus.cend() &&
           (psduIt->startTx < m_firstUlPktsGenTime ||
            psduIt->psduMap.cbegin()->second->GetHeader(0).IsBeacon()))
    {
        ++psduIt;
    }

    // the first QoS data frame is transmitted by the main PHY without RTS protection as soon
    // as transmissions on the link where the main PHY is operating are unblocked (at this
    // moment, aux PHYs cannot transmit)
    NS_TEST_ASSERT_MSG_EQ((psduIt != m_txPsdus.cend()),
                          true,
                          "First QoS data frame has not been transmitted");
    NS_TEST_EXPECT_MSG_EQ(psduIt->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
                          true,
                          "First QoS data frame should be transmitted without protection");
    NS_TEST_EXPECT_MSG_EQ(+psduIt->phyId,
                          +m_mainPhyId,
                          "First QoS data frame should be transmitted by the main PHY");
    NS_TEST_EXPECT_MSG_GT_OR_EQ(psduIt->startTx,
                                m_firstUlPktsGenTime + m_unblockMainPhyLinkDelay,
                                "First QoS data frame sent too early");

    auto prevPsduIt = psduIt++;
    jumpToQosDataOrMuRts();

    if (m_nonEmlsrLink)
    {
        // an additional data frame is sent concurrently on the non-EMLSR link
        NS_TEST_ASSERT_MSG_EQ(
            (psduIt != m_txPsdus.cend()),
            true,
            "Expected another QoS data frame sent concurrently with the first frame");
        NS_TEST_EXPECT_MSG_EQ(
            psduIt->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
            true,
            "First data frame on non-EMLSR link should be transmitted without protection");
        NS_TEST_EXPECT_MSG_EQ(+psduIt->linkId,
                              +m_nonEmlsrLink.value(),
                              "First data frame expected to be transmitted on the non-EMLSR link");
        const auto txDuration =
            WifiPhy::CalculateTxDuration(prevPsduIt->psduMap,
                                         prevPsduIt->txVector,
                                         m_staMacs[0]->GetWifiPhy(prevPsduIt->phyId)->GetPhyBand());
        NS_TEST_EXPECT_MSG_LT(psduIt->startTx,
                              prevPsduIt->startTx + txDuration,
                              "First data frame on the non-EMLSR link not sent concurrently");
        psduIt++;
        jumpToQosDataOrMuRts();
    }

    // the second QoS data frame is transmitted by the main PHY after that the aux PHY has
    // obtained a TXOP and sent an RTS
    // RTS
    NS_TEST_ASSERT_MSG_EQ((psduIt != m_txPsdus.cend()),
                          true,
                          "RTS before second QoS data frame has not been transmitted");
    NS_TEST_EXPECT_MSG_EQ(psduIt->psduMap.cbegin()->second->GetHeader(0).IsRts(),
                          true,
                          "Second QoS data frame should be transmitted with protection");
    NS_TEST_EXPECT_MSG_NE(
        +psduIt->phyId,
        +m_mainPhyId,
        "RTS before second QoS data frame should not be transmitted by the main PHY");
    NS_TEST_EXPECT_MSG_EQ(psduIt->txVector.GetChannelWidth(),
                          m_auxPhyChannelWidth,
                          "RTS before second data frame transmitted on an unexpected width");
    psduIt++;
    // CTS
    NS_TEST_ASSERT_MSG_EQ((psduIt != m_txPsdus.cend()),
                          true,
                          "CTS before second QoS data frame has not been transmitted");
    NS_TEST_EXPECT_MSG_EQ(psduIt->psduMap.cbegin()->second->GetHeader(0).IsCts(),
                          true,
                          "CTS before second QoS data frame has not been transmitted");
    psduIt++;
    // QoS Data
    NS_TEST_ASSERT_MSG_EQ((psduIt != m_txPsdus.cend()),
                          true,
                          "Second QoS data frame has not been transmitted");
    NS_TEST_EXPECT_MSG_EQ(psduIt->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
                          true,
                          "Second QoS data frame has not been transmitted");
    NS_TEST_EXPECT_MSG_EQ(+psduIt->phyId,
                          +m_mainPhyId,
                          "Second QoS data frame should be transmitted by the main PHY");
    NS_TEST_EXPECT_MSG_EQ(psduIt->txVector.GetChannelWidth(),
                          m_auxPhyChannelWidth,
                          "Second data frame not transmitted on the same width as RTS");

    bool moreQosDataFound = false;

    while (++psduIt != m_txPsdus.cend())
    {
        jumpToQosDataOrMuRts();
        if (psduIt != m_txPsdus.cend() &&
            psduIt->psduMap.cbegin()->second->GetHeader(0).IsQosData())
        {
            moreQosDataFound = true;

            NS_TEST_EXPECT_MSG_EQ(+psduIt->phyId,
                                  +m_mainPhyId,
                                  "Third QoS data frame should be transmitted by the main PHY");
            NS_TEST_EXPECT_MSG_EQ(psduIt->txVector.GetChannelWidth(),
                                  m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId)->GetChannelWidth(),
                                  "Expecting TX width of third data frame to equal the channel "
                                  "width used by the main PHY");
            NS_TEST_EXPECT_MSG_GT_OR_EQ(
                psduIt->startTx,
                m_lastMsdExpiryTime,
                "Third QoS data frame sent before MediumSyncDelay timer expired");

            break;
        }
    }

    NS_TEST_EXPECT_MSG_EQ(moreQosDataFound,
                          true,
                          "Third QoS data frame transmitted by the main PHY not found");

    NS_TEST_ASSERT_MSG_EQ((psduIt != m_txPsdus.cend()), true, "Expected more frames");
    ++psduIt;
    jumpToQosDataOrMuRts();

    // the first attempt at transmitting the fourth QoS data frame fails because CTS is corrupted
    // RTS
    NS_TEST_ASSERT_MSG_EQ((psduIt != m_txPsdus.cend()),
                          true,
                          "RTS before fourth QoS data frame has not been transmitted");
    NS_TEST_EXPECT_MSG_EQ(psduIt->psduMap.cbegin()->second->GetHeader(0).IsRts(),
                          true,
                          "Fourth QoS data frame should be transmitted with protection");
    NS_TEST_EXPECT_MSG_NE(
        +psduIt->phyId,
        +m_mainPhyId,
        "RTS before fourth QoS data frame should not be transmitted by the main PHY");
    NS_TEST_EXPECT_MSG_EQ(psduIt->txVector.GetChannelWidth(),
                          m_auxPhyChannelWidth,
                          "RTS before fourth data frame transmitted on an unexpected width");
    psduIt++;
    // CTS
    NS_TEST_ASSERT_MSG_EQ((psduIt != m_txPsdus.cend()),
                          true,
                          "CTS before fourth QoS data frame has not been transmitted");
    NS_TEST_EXPECT_MSG_EQ(psduIt->psduMap.cbegin()->second->GetHeader(0).IsCts(),
                          true,
                          "CTS before fourth QoS data frame has not been transmitted");
    psduIt++;
    jumpToQosDataOrMuRts();

    // the fourth QoS data frame is transmitted by an aux PHY after that the aux PHY has
    // obtained a TXOP and sent an RTS
    // RTS
    NS_TEST_ASSERT_MSG_EQ((psduIt != m_txPsdus.cend()),
                          true,
                          "RTS before fourth QoS data frame has not been transmitted");
    NS_TEST_EXPECT_MSG_EQ(psduIt->psduMap.cbegin()->second->GetHeader(0).IsRts(),
                          true,
                          "Fourth QoS data frame should be transmitted with protection");
    NS_TEST_EXPECT_MSG_NE(
        +psduIt->phyId,
        +m_mainPhyId,
        "RTS before fourth QoS data frame should not be transmitted by the main PHY");
    NS_TEST_EXPECT_MSG_EQ(psduIt->txVector.GetChannelWidth(),
                          m_auxPhyChannelWidth,
                          "RTS before fourth data frame transmitted on an unexpected width");
    psduIt++;
    // CTS
    NS_TEST_ASSERT_MSG_EQ((psduIt != m_txPsdus.cend()),
                          true,
                          "CTS before fourth QoS data frame has not been transmitted");
    NS_TEST_EXPECT_MSG_EQ(psduIt->psduMap.cbegin()->second->GetHeader(0).IsCts(),
                          true,
                          "CTS before fourth QoS data frame has not been transmitted");
    psduIt++;
    // QoS Data
    NS_TEST_ASSERT_MSG_EQ((psduIt != m_txPsdus.cend()),
                          true,
                          "Fourth QoS data frame has not been transmitted");
    NS_TEST_EXPECT_MSG_EQ(psduIt->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
                          true,
                          "Fourth QoS data frame has not been transmitted");
    NS_TEST_EXPECT_MSG_EQ(+psduIt->phyId,
                          +m_mainPhyId,
                          "Fourth QoS data frame should be transmitted by the main PHY");
    NS_TEST_EXPECT_MSG_EQ(psduIt->txVector.GetChannelWidth(),
                          m_auxPhyChannelWidth,
                          "Fourth data frame not transmitted on the same width as RTS");

    auto fourthLinkId = psduIt->linkId;

    psduIt++;
    jumpToQosDataOrMuRts();

    NS_TEST_ASSERT_MSG_EQ((psduIt != m_txPsdus.cend()), true, "Expected more frames");
    // Do not check the start transmission time if a backoff is generated even when no
    // transmission is done (if the backoff expires while the main PHY is switching, a new
    // backoff is generated and, before this backoff expires, the main PHY may be requested
    // to switch to another auxiliary link; this may happen multiple times...)
    if (!m_genBackoffIfTxopWithoutTx && !m_switchMainPhyBackDelayTimeout)
    {
        NS_TEST_EXPECT_MSG_LT_OR_EQ(psduIt->startTx,
                                    m_5thQosFrameTxTime,
                                    "Fifth data frame transmitted too late");
    }

    // the fifth QoS data frame is transmitted by the main PHY on an auxiliary link because
    // the aux PHY is not TX capable. The QoS data frame is protected by RTS if it is transmitted
    // on a different link than the previous one (because the MediumSyncDelay timer is running)
    if (psduIt->linkId != fourthLinkId)
    {
        // RTS
        NS_TEST_EXPECT_MSG_EQ(psduIt->psduMap.cbegin()->second->GetHeader(0).IsRts(),
                              true,
                              "Fifth QoS data frame should be transmitted with protection");
        NS_TEST_EXPECT_MSG_EQ(
            +psduIt->phyId,
            +m_mainPhyId,
            "RTS before fifth QoS data frame should be transmitted by the main PHY");
        psduIt++;
        // CTS
        NS_TEST_ASSERT_MSG_EQ((psduIt != m_txPsdus.cend()),
                              true,
                              "CTS before fifth QoS data frame has not been transmitted");
        NS_TEST_EXPECT_MSG_EQ(psduIt->psduMap.cbegin()->second->GetHeader(0).IsCts(),
                              true,
                              "CTS before fifth QoS data frame has not been transmitted");
        psduIt++;
    }

    // QoS Data
    NS_TEST_ASSERT_MSG_EQ((psduIt != m_txPsdus.cend()),
                          true,
                          "Fifth QoS data frame has not been transmitted");
    NS_TEST_EXPECT_MSG_EQ(psduIt->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
                          true,
                          "Fifth QoS data frame has not been transmitted");
    NS_TEST_EXPECT_MSG_EQ(+psduIt->phyId,
                          +m_mainPhyId,
                          "Fifth QoS data frame should be transmitted by the main PHY");
    NS_TEST_EXPECT_MSG_NE(+psduIt->linkId,
                          +m_mainPhyId,
                          "Fifth QoS data frame should be transmitted on an auxiliary link");
    NS_TEST_EXPECT_MSG_EQ(psduIt->txVector.GetChannelWidth(),
                          m_5thQosFrameExpWidth,
                          "Fifth data frame not transmitted on the correct channel width");
}

EmlsrUlOfdmaTest::EmlsrUlOfdmaTest(bool enableBsrp)
    : EmlsrOperationsTestBase("Check UL OFDMA operations with an EMLSR client"),
      m_enableBsrp(enableBsrp),
      m_txPsdusPos(0),
      m_startAccessReq(0)
{
    m_linksToEnableEmlsrOn = {0, 1, 2};
    m_nEmlsrStations = 1;
    m_nNonEmlsrStations = 1;
    m_establishBaDl = false;
    m_establishBaUl = true;
    m_mainPhyId = 1;
    m_duration = Seconds(1);
}

void
EmlsrUlOfdmaTest::DoSetup()
{
    Config::SetDefault("ns3::WifiPhy::ChannelSwitchDelay", TimeValue(m_transitionDelay.at(0)));

    EmlsrOperationsTestBase::DoSetup();

    m_apMac->GetQosTxop(AC_BE)->SetTxopLimits(
        {MicroSeconds(3200), MicroSeconds(3200), MicroSeconds(3200)});

    auto muScheduler = CreateObjectWithAttributes<RrMultiUserScheduler>("EnableUlOfdma",
                                                                        BooleanValue(true),
                                                                        "EnableBsrp",
                                                                        BooleanValue(m_enableBsrp));
    m_apMac->AggregateObject(muScheduler);
}

void
EmlsrUlOfdmaTest::Transmit(Ptr<WifiMac> mac,
                           uint8_t phyId,
                           WifiConstPsduMap psduMap,
                           WifiTxVector txVector,
                           double txPowerW)
{
    EmlsrOperationsTestBase::Transmit(mac, phyId, psduMap, txVector, txPowerW);
    auto linkId = m_txPsdus.back().linkId;

    auto psdu = psduMap.begin()->second;

    switch (psdu->GetHeader(0).GetType())
    {
    case WIFI_MAC_CTL_TRIGGER:
        if (m_txPsdusPos == 0 && !m_startAccessReq.IsZero() && Simulator::Now() >= m_startAccessReq)
        {
            // this is the first Trigger Frame sent after the AP requested channel access
            // through the Multi-user scheduler and it is an ICF for the EMLSR client
            m_txPsdusPos = m_txPsdus.size() - 1;
            auto txDuration = WifiPhy::CalculateTxDuration(psduMap,
                                                           txVector,
                                                           mac->GetWifiPhy(linkId)->GetPhyBand());
            NS_LOG_INFO("This is the first Trigger Frame\n");
            // once the Trigger Frame is received by the EMLSR client, make the client application
            // on the EMLSR client generate two packets. These packets will be sent via UL OFDMA
            // because the EMLSR client has blocked transmissions on other links when receiving
            // this Trigger Frame, hence it will not try to get access on other links via EDCA
            Simulator::Schedule(
                txDuration + MicroSeconds(1), // to account for propagation delay
                [=, this]() {
                    for (const auto id : m_staMacs[0]->GetLinkIds())
                    {
                        auto ehtFem = StaticCast<EhtFrameExchangeManager>(
                            m_staMacs[0]->GetFrameExchangeManager(id));
                        NS_TEST_EXPECT_MSG_EQ(
                            ehtFem->UsingOtherEmlsrLink(),
                            (id != linkId),
                            "Link " << +id << " was" << (id == linkId ? " not" : "")
                                    << " expected to be blocked on EMLSR client at time "
                                    << Simulator::Now().As(Time::NS));
                    }
                    NS_LOG_INFO("Generate two packets\n");
                    m_staMacs[0]->GetDevice()->GetNode()->AddApplication(
                        GetApplication(UPLINK, 0, 2, 100));
                });
        }
        break;

    case WIFI_MAC_CTL_BACKRESP:
        if (!m_startAccessReq.IsZero() && Simulator::Now() >= m_startAccessReq)
        {
            CtrlBAckResponseHeader blockAck;
            psdu->GetPayload(0)->PeekHeader(blockAck);
            if (blockAck.IsMultiSta())
            {
                auto txDuration =
                    WifiPhy::CalculateTxDuration(psduMap,
                                                 txVector,
                                                 mac->GetWifiPhy(linkId)->GetPhyBand());
                Simulator::Stop(txDuration + MicroSeconds(1));
            }
        }
        break;

    default:;
    }

    if (psdu->GetHeader(0).IsCfEnd())
    {
        // we do not check CF-End frames
        m_txPsdus.pop_back();
    }
}

void
EmlsrUlOfdmaTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

void
EmlsrUlOfdmaTest::StartTraffic()
{
    auto muScheduler = m_apMac->GetObject<MultiUserScheduler>();
    NS_TEST_ASSERT_MSG_NE(muScheduler, nullptr, "No MU scheduler installed on AP MLD");

    NS_LOG_INFO("Setting Access Request interval");

    const auto interval = MilliSeconds(50);
    muScheduler->SetAccessReqInterval(interval);
    m_startAccessReq = Simulator::Now() + interval;
}

void
EmlsrUlOfdmaTest::CheckResults()
{
    /**
     * Sending BSRP TF disabled.
     *
     * The figure assumes that link 0 is used to send the first Trigger Frame after that the
     * AP MLD requests channel access through the Multi-user scheduler. The first Trigger Frame
     * is MU-RTS because EMLSR client needs an ICF; the other Trigger Frames are Basic TFs and
     * do not solicit the EMLSR client.
     *             ┌─────┐     ┌─────┐           ┌──────┐
     *             │ MU  │     │Basic│           │Multi-│
     *  [link 0]   │ RTS │     │  TF │           │STA BA│
     *  ───────────┴─────┴┬───┬┴─────┴┬────────┬─┴──────┴───────────────
     *                    │CTS│       │QoS Null│
     *                    ├───┤       ├────────┤
     *                    │CTS│       │QoS Data│
     *                    └───┘       └────────┘
     *
     *               ┌─────┐
     *               │Basic│
     *  [link 1]     │  TF │
     *  ─────────────┴─────┴┬────┬──────────────────────────────────────
     *                      │QoS │
     *                      │Null│
     *                      └────┘
     *
     *               ┌─────┐
     *               │Basic│
     *  [link 2]     │  TF │
     *  ─────────────┴─────┴┬────┬──────────────────────────────────────
     *                      │QoS │
     *                      │Null│
     *                      └────┘
     *
     * Sending BSRP TF enabled.
     *
     * The figure assumes that link 0 is used to send the first Trigger Frame after that the
     * AP MLD requests channel access through the Multi-user scheduler. The first Trigger Frames
     * are all BSRP Trigger Frames, but only the first one solicits the EMLSR client, too.
     *             ┌─────┐          ┌─────┐           ┌──────┐
     *             │BSRP │          │Basic│           │Multi-│
     *  [link 0]   │  TF │          │  TF │           │STA BA│
     *  ───────────┴─────┴┬────────┬┴─────┴┬────────┬─┴──────┴──────────
     *                    │QoS Null│       │QoS Data│
     *                    ├────────┤       └────────┘
     *                    │QoS Null│
     *                    └────────┘
     *
     *               ┌─────┐
     *               │BSRP │
     *  [link 1]     │  TF │
     *  ─────────────┴─────┴┬────┬──────────────────────────────────────
     *                      │QoS │
     *                      │Null│
     *                      └────┘
     *
     *               ┌─────┐
     *               │BSRP │
     *  [link 2]     │  TF │
     *  ─────────────┴─────┴┬────┬──────────────────────────────────────
     *                      │QoS │
     *                      │Null│
     *                      └────┘
     */

    NS_TEST_ASSERT_MSG_GT(m_txPsdusPos, 0, "First Trigger Frame not detected");

    // Check the Trigger Frames (one per link) after requesting channel access
    auto index = m_txPsdusPos;
    const auto firstLinkId = m_txPsdus[m_txPsdusPos].linkId;
    for (; index < m_txPsdusPos + 3; ++index)
    {
        NS_TEST_ASSERT_MSG_EQ(m_txPsdus[index].psduMap.cbegin()->second->GetHeader(0).IsTrigger(),
                              true,
                              "Expected a Trigger Frame");
        CtrlTriggerHeader trigger;
        m_txPsdus[index].psduMap.cbegin()->second->GetPayload(0)->PeekHeader(trigger);

        TriggerFrameType triggerType =
            m_enableBsrp ? TriggerFrameType::BSRP_TRIGGER
                         : (index == m_txPsdusPos ? TriggerFrameType::MU_RTS_TRIGGER
                                                  : TriggerFrameType::BASIC_TRIGGER);
        NS_TEST_EXPECT_MSG_EQ(+static_cast<uint8_t>(trigger.GetType()),
                              +static_cast<uint8_t>(triggerType),
                              "Unexpected Trigger Frame type on link " << +m_txPsdus[index].linkId);

        // only the first TF solicits the EMLSR client and the non-AP MLD
        NS_TEST_EXPECT_MSG_EQ(
            trigger.GetNUserInfoFields(),
            (index == m_txPsdusPos ? 2 : 1),
            "Unexpected number of User Info fields for Trigger Frame, index=" << index);
    }

    auto startIndex = index;
    std::size_t ctsCount = 0;
    std::size_t qosNullCount = 0;
    // Check responses to Trigger Frames
    for (; index < startIndex + 4; ++index)
    {
        const auto& hdr = m_txPsdus[index].psduMap.cbegin()->second->GetHeader(0);

        if (hdr.IsCts())
        {
            ++ctsCount;
            continue;
        }

        if (hdr.IsQosData() && !hdr.HasData())
        {
            ++qosNullCount;
            // if BSRP is enabled, the QoS Null frame sent by the EMLSR client in response to the
            // first BSRP TF reports a non-null buffer status
            if (m_enableBsrp &&
                hdr.GetAddr2() == m_staMacs[0]->GetFrameExchangeManager(firstLinkId)->GetAddress())
            {
                NS_TEST_EXPECT_MSG_GT(+hdr.GetQosQueueSize(), 0, "Unexpected buffer size");
            }
            else
            {
                NS_TEST_EXPECT_MSG_EQ(+hdr.GetQosQueueSize(), 0, "Unexpected buffer size");
            }
            continue;
        }
    }
    NS_TEST_EXPECT_MSG_EQ(ctsCount, (m_enableBsrp ? 0 : 2), "Unexpected number of CTS frames");
    NS_TEST_EXPECT_MSG_EQ(qosNullCount,
                          (m_enableBsrp ? 4 : 2),
                          "Unexpected number of QoS Null frames");

    // we expect only one Basic Trigger Frame (sent on the same link as the first Trigger Frame),
    // because the buffer status reported on the other links by the non-EMLSR client is zero
    NS_TEST_ASSERT_MSG_EQ(m_txPsdus[index].psduMap.cbegin()->second->GetHeader(0).IsTrigger(),
                          true,
                          "Expected a Trigger Frame");
    NS_TEST_EXPECT_MSG_EQ(+m_txPsdus[index].linkId,
                          +firstLinkId,
                          "Unexpected link ID for Basic TF");
    CtrlTriggerHeader trigger;
    m_txPsdus[index].psduMap.cbegin()->second->GetPayload(0)->PeekHeader(trigger);

    NS_TEST_EXPECT_MSG_EQ(+static_cast<uint8_t>(trigger.GetType()),
                          +static_cast<uint8_t>(TriggerFrameType::BASIC_TRIGGER),
                          "Unexpected Trigger Frame type");

    // when BSRP TF is enabled, the non-EMLSR client has already communicated a buffer status of
    // zero, so it is not solicited by the AP through the Basic Trigger Frame. Otherwise, it is
    // solicited because buffer status was not known when the BSRP TF was prepared (before sending
    // MU-RTS)
    NS_TEST_EXPECT_MSG_EQ(trigger.GetNUserInfoFields(),
                          (m_enableBsrp ? 1 : 2),
                          "Unexpected number of User Info fields for Basic Trigger Frame");

    // Response(s) to the Basic Trigger Frame
    startIndex = ++index;
    for (; index < startIndex + (m_enableBsrp ? 1 : 2); ++index)
    {
        const auto& hdr = m_txPsdus[index].psduMap.cbegin()->second->GetHeader(0);

        NS_TEST_EXPECT_MSG_EQ(hdr.IsQosData(), true, "Expected a QoS frame");

        // EMLSR client sends a QoS Data frame, non-EMLSR client sends a QoS Null frame
        NS_TEST_EXPECT_MSG_EQ(
            hdr.HasData(),
            (hdr.GetAddr2() == m_staMacs[0]->GetFrameExchangeManager(firstLinkId)->GetAddress()),
            "Unexpected type of QoS data frame");

        if (hdr.HasData())
        {
            NS_TEST_EXPECT_MSG_EQ(m_txPsdus[index].txVector.IsUlMu(),
                                  true,
                                  "QoS Data frame should be sent in a TB PPDU");
        }
    }

    // Finally, the AP MLD sends a Multi-STA BlockAck
    NS_TEST_EXPECT_MSG_EQ(m_txPsdus[index].psduMap.cbegin()->second->GetHeader(0).IsBlockAck(),
                          true,
                          "Expected a BlockAck frame");
    CtrlBAckResponseHeader blockAck;
    m_txPsdus[index].psduMap.cbegin()->second->GetPayload(0)->PeekHeader(blockAck);
    NS_TEST_EXPECT_MSG_EQ(blockAck.IsMultiSta(), true, "Expected a Multi-STA BlockAck");
}

EmlsrLinkSwitchTest::EmlsrLinkSwitchTest(const Params& params)
    : EmlsrOperationsTestBase(
          std::string("Check EMLSR link switching (switchAuxPhy=") +
          std::to_string(params.switchAuxPhy) + ", resetCamStateAndInterruptSwitch=" +
          std::to_string(params.resetCamStateAndInterruptSwitch) +
          ", auxPhyMaxChWidth=" + std::to_string(params.auxPhyMaxChWidth) + "MHz )"),
      m_switchAuxPhy(params.switchAuxPhy),
      m_resetCamStateAndInterruptSwitch(params.resetCamStateAndInterruptSwitch),
      m_auxPhyMaxChWidth(params.auxPhyMaxChWidth),
      m_countQoSframes(0),
      m_countIcfFrames(0),
      m_countRtsFrames(0),
      m_txPsdusPos(0)
{
    m_nEmlsrStations = 1;
    m_nNonEmlsrStations = 0;
    m_linksToEnableEmlsrOn = {0, 1, 2}; // enable EMLSR on all links right after association
    m_mainPhyId = 1;
    m_establishBaDl = true;
    m_duration = Seconds(1);
    // when aux PHYs do not switch link, the main PHY switches back to its previous link after
    // a TXOP, hence the transition delay must exceed the channel switch delay (default: 250us)
    m_transitionDelay = {MicroSeconds(128)};
}

void
EmlsrLinkSwitchTest::Transmit(Ptr<WifiMac> mac,
                              uint8_t phyId,
                              WifiConstPsduMap psduMap,
                              WifiTxVector txVector,
                              double txPowerW)
{
    EmlsrOperationsTestBase::Transmit(mac, phyId, psduMap, txVector, txPowerW);
    auto linkId = m_txPsdus.back().linkId;

    auto psdu = psduMap.begin()->second;
    auto nodeId = mac->GetDevice()->GetNode()->GetId();

    switch (psdu->GetHeader(0).GetType())
    {
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
        NS_ASSERT_MSG(nodeId > 0, "APs do not send AssocReq frames");
        NS_TEST_EXPECT_MSG_EQ(+linkId, +m_mainPhyId, "AssocReq not sent by the main PHY");
        break;

    case WIFI_MAC_MGT_ACTION: {
        auto [category, action] = WifiActionHeader::Peek(psdu->GetPayload(0));

        if (nodeId == 1 && category == WifiActionHeader::PROTECTED_EHT &&
            action.protectedEhtAction ==
                WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION)
        {
            // the EMLSR client is starting the transmission of the EML OMN frame;
            // temporarily block transmissions of QoS data frames from the AP MLD to the
            // non-AP MLD on all the links but the one used for ML setup, so that we know
            // that the first QoS data frame is sent on the link of the main PHY
            std::set<uint8_t> linksToBlock;
            for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
            {
                if (id != m_mainPhyId)
                {
                    linksToBlock.insert(id);
                }
            }
            m_apMac->GetMacQueueScheduler()->BlockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                         AC_BE,
                                                         {WIFI_QOSDATA_QUEUE},
                                                         m_staMacs[0]->GetAddress(),
                                                         m_apMac->GetAddress(),
                                                         {0},
                                                         linksToBlock);
        }
    }
    break;

    case WIFI_MAC_CTL_TRIGGER:
        CheckInitialControlFrame(psduMap, txVector, linkId);
        break;

    case WIFI_MAC_QOSDATA:
        CheckQosFrames(psduMap, txVector, linkId);
        break;

    case WIFI_MAC_CTL_RTS:
        CheckRtsFrame(psduMap, txVector, linkId);
        break;

    default:;
    }
}

void
EmlsrLinkSwitchTest::DoSetup()
{
    Config::SetDefault("ns3::DefaultEmlsrManager::SwitchAuxPhy", BooleanValue(m_switchAuxPhy));
    Config::SetDefault("ns3::EmlsrManager::ResetCamState",
                       BooleanValue(m_resetCamStateAndInterruptSwitch));
    Config::SetDefault("ns3::AdvancedEmlsrManager::InterruptSwitch",
                       BooleanValue(m_resetCamStateAndInterruptSwitch));
    Config::SetDefault("ns3::EmlsrManager::AuxPhyChannelWidth", UintegerValue(m_auxPhyMaxChWidth));
    Config::SetDefault("ns3::WifiPhy::ChannelSwitchDelay", TimeValue(MicroSeconds(45)));

    EmlsrOperationsTestBase::DoSetup();

    m_errorModel = CreateObject<ListErrorModel>();
    for (std::size_t linkId = 0; linkId < m_apMac->GetNLinks(); ++linkId)
    {
        m_apMac->GetWifiPhy(linkId)->SetPostReceptionErrorModel(m_errorModel);
    }

    // use channels of different widths
    for (auto mac : std::initializer_list<Ptr<WifiMac>>{m_apMac, m_staMacs[0]})
    {
        mac->GetWifiPhy(0)->SetOperatingChannel(
            WifiPhy::ChannelTuple{4, 40, WIFI_PHY_BAND_2_4GHZ, 1});
        mac->GetWifiPhy(1)->SetOperatingChannel(
            WifiPhy::ChannelTuple{58, 80, WIFI_PHY_BAND_5GHZ, 3});
        mac->GetWifiPhy(2)->SetOperatingChannel(
            WifiPhy::ChannelTuple{79, 160, WIFI_PHY_BAND_6GHZ, 7});
    }
}

void
EmlsrLinkSwitchTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

void
EmlsrLinkSwitchTest::CheckQosFrames(const WifiConstPsduMap& psduMap,
                                    const WifiTxVector& txVector,
                                    uint8_t linkId)
{
    m_countQoSframes++;

    switch (m_countQoSframes)
    {
    case 1:
        // unblock transmissions on all links
        m_apMac->GetMacQueueScheduler()->UnblockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                       AC_BE,
                                                       {WIFI_QOSDATA_QUEUE},
                                                       m_staMacs[0]->GetAddress(),
                                                       m_apMac->GetAddress(),
                                                       {0},
                                                       {0, 1, 2});
        // block transmissions on the link used for ML setup
        m_apMac->GetMacQueueScheduler()->BlockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                     AC_BE,
                                                     {WIFI_QOSDATA_QUEUE},
                                                     m_staMacs[0]->GetAddress(),
                                                     m_apMac->GetAddress(),
                                                     {0},
                                                     {m_mainPhyId});
        // generate a new data packet, which will be sent on a link other than the one
        // used for ML setup, hence triggering a link switching on the EMLSR client
        m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(DOWNLINK, 0, 2, 1000));
        break;
    case 2:
        // block transmission on the link used to send this QoS data frame
        m_apMac->GetMacQueueScheduler()->BlockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                     AC_BE,
                                                     {WIFI_QOSDATA_QUEUE},
                                                     m_staMacs[0]->GetAddress(),
                                                     m_apMac->GetAddress(),
                                                     {0},
                                                     {linkId});
        // generate a new data packet, which will be sent on the link that has not been used
        // so far, hence triggering another link switching on the EMLSR client
        m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(DOWNLINK, 0, 2, 1000));
        break;
    case 3:
        // block transmission on the link used to send this QoS data frame
        m_apMac->GetMacQueueScheduler()->BlockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                     AC_BE,
                                                     {WIFI_QOSDATA_QUEUE},
                                                     m_staMacs[0]->GetAddress(),
                                                     m_apMac->GetAddress(),
                                                     {0},
                                                     {linkId});
        // unblock transmissions on the link used for ML setup
        m_apMac->GetMacQueueScheduler()->UnblockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                       AC_BE,
                                                       {WIFI_QOSDATA_QUEUE},
                                                       m_staMacs[0]->GetAddress(),
                                                       m_apMac->GetAddress(),
                                                       {0},
                                                       {m_mainPhyId});
        // generate a new data packet, which will be sent again on the link used for ML setup,
        // hence triggering yet another link switching on the EMLSR client
        m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(DOWNLINK, 0, 2, 1000));
        break;
    case 4:
        // block transmissions on all links at non-AP MLD side
        m_staMacs[0]->GetMacQueueScheduler()->BlockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                          AC_BE,
                                                          {WIFI_QOSDATA_QUEUE},
                                                          m_apMac->GetAddress(),
                                                          m_staMacs[0]->GetAddress(),
                                                          {0},
                                                          {0, 1, 2});
        // unblock transmissions on the link used for ML setup (non-AP MLD side)
        m_staMacs[0]->GetMacQueueScheduler()->UnblockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                            AC_BE,
                                                            {WIFI_QOSDATA_QUEUE},
                                                            m_apMac->GetAddress(),
                                                            m_staMacs[0]->GetAddress(),
                                                            {0},
                                                            {m_mainPhyId});
        // trigger establishment of BA agreement with AP as recipient
        m_staMacs[0]->GetDevice()->GetNode()->AddApplication(GetApplication(UPLINK, 0, 4, 1000));
        break;
    case 5:
        // unblock transmissions on all links at non-AP MLD side
        m_staMacs[0]->GetMacQueueScheduler()->UnblockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                            AC_BE,
                                                            {WIFI_QOSDATA_QUEUE},
                                                            m_apMac->GetAddress(),
                                                            m_staMacs[0]->GetAddress(),
                                                            {0},
                                                            {0, 1, 2});
        // block transmissions on the link used for ML setup (non-AP MLD side)
        m_staMacs[0]->GetMacQueueScheduler()->BlockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                          AC_BE,
                                                          {WIFI_QOSDATA_QUEUE},
                                                          m_apMac->GetAddress(),
                                                          m_staMacs[0]->GetAddress(),
                                                          {0},
                                                          {m_mainPhyId});
        // generate a new data packet, which will be sent on a link other than the one
        // used for ML setup, hence triggering a link switching on the EMLSR client
        m_staMacs[0]->GetDevice()->GetNode()->AddApplication(GetApplication(UPLINK, 0, 2, 1000));
        break;
    }
}

/**
 * AUX PHY switching enabled (X = channel switch delay)
 *
 *  |--------- aux PHY A ---------|------ main PHY ------|-------------- aux PHY B -------------
 *                           ┌───┐     ┌───┐
 *                           │ICF│     │QoS│
 * ──────────────────────────┴───┴┬───┬┴───┴┬──┬────────────────────────────────────────────────
 *  [link 0]                      │CTS│     │BA│
 *                                └───┘     └──┘
 *
 *
 *  |--------- main PHY ----------|------------------ aux PHY A ----------------|--- main PHY ---
 *     ┌───┐     ┌───┐                                                      ┌───┐     ┌───┐
 *     │ICF│     │QoS│                                                      │ICF│     │QoS│
 *  ───┴───┴┬───┬┴───┴┬──┬──────────────────────────────────────────────────┴───┴┬───┬┴───┴┬──┬──
 *  [link 1]│CTS│     │BA│                                                       │CTS│     │BA│
 *          └───┘     └──┘                                                       └───┘     └──┘
 *
 *
 *  |--------------------- aux PHY B --------------------|------ main PHY ------|-- aux PHY A ---
 *                                                   ┌───┐     ┌───┐
 *                                                   │ICF│     │QoS│
 *  ─────────────────────────────────────────────────┴───┴┬───┬┴───┴┬──┬─────────────────────────
 *  [link 2]                                              │CTS│     │BA│
 *                                                        └───┘     └──┘
 *
 * ... continued ...
 *
 *  |----------------------------------------- aux PHY B ---------------------------------------
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 *  [link 0]
 *
 *  |--------- main PHY ----------|X|X|------------------------ aux PHY A ----------------------
 *                 ┌───┐
 *                 │ACK│
 *  ──────────┬───┬┴───┴────────────────────────────────────────────────────────────────────────
 *  [link 1]  │QoS│
 *            └───┘
 *
 *  |-------- aux PHY A ----------|X|---------------------- main PHY ---------------------------
 *                                          ┌──┐
 *                                          │BA│
 *  ────────────────────────┬───X──────┬───┬┴──┴────────────────────────────────────────────────
 *  [link 2]                │RTS│      │QoS│
 *                          └───┘      └───┘
 ************************************************************************************************
 *
 * AUX PHY switching disabled (X = channel switch delay)
 *
 *  |------------------------------------------ aux PHY A ---------------------------------------
 *                                |-- main PHY --|X|
 *                            ┌───┐     ┌───┐
 *                            │ICF│     │QoS│
 *  ──────────────────────────┴───┴┬───┬┴───┴┬──┬────────────────────────────────────────────────
 *  [link 0]                       │CTS│     │BA│
 *                                 └───┘     └──┘
 *
 *                                                 |-main|
 *  |--------- main PHY ----------|                |-PHY-|                |------ main PHY ------
 *     ┌───┐     ┌───┐                                                      ┌───┐     ┌───┐
 *     │ICF│     │QoS│                                                      │ICF│     │QoS│
 *  ───┴───┴┬───┬┴───┴┬──┬──────────────────────────────────────────────────┴───┴┬───┬┴───┴┬──┬──
 *  [link 1]│CTS│     │BA│                                                       │CTS│     │BA│
 *          └───┘     └──┘                                                       └───┘     └──┘
 *
 *
 *  |------------------------------------------ aux PHY B ---------------------------------------
 *                                                       |-- main PHY --|X|
 *                                                   ┌───┐     ┌───┐
 *                                                   │ICF│     │QoS│
 *  ─────────────────────────────────────────────────┴───┴┬───┬┴───┴┬──┬─────────────────────────
 *  [link 2]                                              │CTS│     │BA│
 *                                                        └───┘     └──┘
 *
 * ... continued ...
 *
 *  |----------------------------------------- aux PHY A ---------------------------------------
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 *  [link 0]
 *
 *  |-------- main PHY --------|      |--- main PHY ---|
 *                 ┌───┐
 *                 │ACK│
 *  ──────────┬───┬┴───┴────────────────────────────────────────────────────────────────────────
 *  [link 1]  │QoS│
 *            └───┘
 *
 *  |------------------------------------------ aux PHY B --------------------------------------
 *                              |X||X|                 |X|-------------- main PHY --------------
 *                                                   ┌───┐     ┌──┐
 *                                                   │CTS│     │BA│
 *  ────────────────────────┬───X───────────────┬───┬┴───┴┬───┬┴──┴─────────────────────────────
 *  [link 2]                │RTS│               │RTS│     │QoS│
 *                          └───┘               └───┘     └───┘
 *
 */

void
EmlsrLinkSwitchTest::CheckInitialControlFrame(const WifiConstPsduMap& psduMap,
                                              const WifiTxVector& txVector,
                                              uint8_t linkId)
{
    if (++m_countIcfFrames == 1)
    {
        m_txPsdusPos = m_txPsdus.size() - 1;
    }

    // the first ICF is sent to protect ADDBA_REQ for DL BA agreement, then one ICF is sent before
    // each of the 4 DL QoS Data frames; finally, another ICF is sent before the ADDBA_RESP for UL
    // BA agreement. Hence, at any time the number of ICF sent is always greater than or equal to
    // the number of QoS data frames sent.
    NS_TEST_EXPECT_MSG_GT_OR_EQ(m_countIcfFrames, m_countQoSframes, "Unexpected number of ICFs");

    auto mainPhy = m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId);
    auto phyRecvIcf = m_staMacs[0]->GetWifiPhy(linkId); // PHY receiving the ICF

    auto currMainPhyLinkId = m_staMacs[0]->GetLinkForPhy(mainPhy);
    NS_TEST_ASSERT_MSG_EQ(currMainPhyLinkId.has_value(),
                          true,
                          "Didn't find the link on which the Main PHY is operating");
    NS_TEST_ASSERT_MSG_NE(phyRecvIcf,
                          nullptr,
                          "No PHY on the link where ICF " << m_countQoSframes << " was sent");

    if (phyRecvIcf != mainPhy)
    {
        NS_TEST_EXPECT_MSG_LT_OR_EQ(
            phyRecvIcf->GetChannelWidth(),
            m_auxPhyMaxChWidth,
            "Aux PHY that received ICF "
                << m_countQoSframes << " is operating on a channel whose width exceeds the limit");
    }

    // the first two ICFs (before ADDBA_REQ and before first DL QoS Data) and the ICF before the
    // ADDBA_RESP are received by the main PHY. If aux PHYs do not switch links, the ICF before
    // the last DL QoS Data is also received by the main PHY
    NS_TEST_EXPECT_MSG_EQ((phyRecvIcf == mainPhy),
                          (m_countIcfFrames == 1 || m_countIcfFrames == 2 ||
                           (!m_switchAuxPhy && m_countIcfFrames == 5) || m_countIcfFrames == 6),
                          "Expecting that the ICF was received by the main PHY");

    // if aux PHYs do not switch links, the main PHY is operating on its original link when
    // the transmission of an ICF starts
    NS_TEST_EXPECT_MSG_EQ(m_switchAuxPhy || currMainPhyLinkId == m_mainPhyId,
                          true,
                          "Main PHY is operating on an unexpected link ("
                              << +currMainPhyLinkId.value() << ", expected " << +m_mainPhyId
                              << ")");

    auto txDuration =
        WifiPhy::CalculateTxDuration(psduMap, txVector, m_apMac->GetWifiPhy(linkId)->GetPhyBand());

    // check that PHYs are operating on the expected link after the reception of the ICF
    Simulator::Schedule(txDuration + NanoSeconds(1), [=, this]() {
        // the main PHY must be operating on the link where ICF was sent
        NS_TEST_EXPECT_MSG_EQ(m_staMacs[0]->GetWifiPhy(linkId),
                              mainPhy,
                              "PHY operating on link where ICF was sent is not the main PHY");

        // the behavior of Aux PHYs depends on whether they switch channel or not
        if (m_switchAuxPhy)
        {
            if (mainPhy != phyRecvIcf)
            {
                NS_TEST_EXPECT_MSG_EQ(phyRecvIcf->IsStateSwitching(),
                                      true,
                                      "Aux PHY expected to switch channel");
            }
            Simulator::Schedule(phyRecvIcf->GetChannelSwitchDelay(), [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_staMacs[0]->GetWifiPhy(*currMainPhyLinkId),
                                      phyRecvIcf,
                                      "The Aux PHY that received the ICF is expected to operate "
                                      "on the link where Main PHY was before switching channel");
            });
        }
        else
        {
            NS_TEST_EXPECT_MSG_EQ(phyRecvIcf->IsStateSwitching(),
                                  false,
                                  "Aux PHY is not expected to switch channel");
            NS_TEST_EXPECT_MSG_EQ(phyRecvIcf->GetPhyBand(),
                                  mainPhy->GetPhyBand(),
                                  "The Aux PHY that received the ICF is expected to operate "
                                  "on the same band as the Main PHY");
        }
    });
}

void
EmlsrLinkSwitchTest::CheckRtsFrame(const WifiConstPsduMap& psduMap,
                                   const WifiTxVector& txVector,
                                   uint8_t linkId)
{
    // corrupt the first RTS frame (sent by the EMLSR client)
    if (++m_countRtsFrames == 1)
    {
        auto psdu = psduMap.begin()->second;
        m_errorModel->SetList({psdu->GetPacket()->GetUid()});

        // check that when CTS timeout occurs, the main PHY is switching
        Simulator::Schedule(
            m_staMacs[0]->GetFrameExchangeManager(linkId)->GetWifiTxTimer().GetDelayLeft() -
                TimeStep(1),
            [=, this]() {
                // store the time to complete the current channel switch at CTS timeout
                auto mainPhy = m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId);
                auto toCurrSwitchEnd = mainPhy->GetDelayUntilIdle() + TimeStep(1);

                Simulator::Schedule(TimeStep(1), [=, this]() {
                    NS_TEST_EXPECT_MSG_EQ(mainPhy->IsStateSwitching(),
                                          true,
                                          "Main PHY expected to be in SWITCHING state instead of "
                                              << mainPhy->GetState()->GetState());

                    // If main PHY channel switch can be interrupted, the main PHY should be back
                    // operating on the preferred link after a channel switch delay. Otherwise, it
                    // will be operating on the preferred link, if SwitchAuxPhy is false, or on the
                    // link used to send the RTS, if SwitchAuxPhy is true, after the remaining
                    // channel switching time plus the channel switch delay.
                    auto newLinkId = (m_resetCamStateAndInterruptSwitch || !m_switchAuxPhy)
                                         ? m_mainPhyId
                                         : linkId;
                    auto delayLeft = m_resetCamStateAndInterruptSwitch
                                         ? Time{0}
                                         : toCurrSwitchEnd; // time to complete current switch
                    if (m_resetCamStateAndInterruptSwitch || !m_switchAuxPhy)
                    {
                        // add the time to perform another channel switch
                        delayLeft += mainPhy->GetChannelSwitchDelay();
                    }

                    auto totalSwitchDelay =
                        delayLeft + (mainPhy->GetChannelSwitchDelay() - toCurrSwitchEnd);

                    Simulator::Schedule(delayLeft - TimeStep(1), [=, this]() {
                        // check if the MSD timer was running on the link left by the main PHY
                        // before completing channel switch
                        bool msdWasRunning = m_staMacs[0]
                                                 ->GetEmlsrManager()
                                                 ->GetElapsedMediumSyncDelayTimer(m_mainPhyId)
                                                 .has_value();

                        Simulator::Schedule(TimeStep(2), [=, this]() {
                            auto id = m_staMacs[0]->GetLinkForPhy(mainPhy);
                            NS_TEST_EXPECT_MSG_EQ(id.has_value(),
                                                  true,
                                                  "Expected main PHY to operate on a link");
                            NS_TEST_EXPECT_MSG_EQ(*id,
                                                  newLinkId,
                                                  "Main PHY is operating on an unexpected link");
                            const auto startMsd =
                                (totalSwitchDelay >
                                 MicroSeconds(EmlsrManager::MEDIUM_SYNC_THRESHOLD_USEC));
                            const auto msdIsRunning = msdWasRunning || startMsd;
                            CheckMsdTimerRunning(
                                m_staMacs[0],
                                m_mainPhyId,
                                msdIsRunning,
                                std::string("because total switch delay was ") +
                                    std::to_string(totalSwitchDelay.GetNanoSeconds()) + "ns");
                        });
                    });
                });
            });
    }
    // block transmissions on all other links at non-AP MLD side
    std::set<uint8_t> links{0, 1, 2};
    links.erase(linkId);
    m_staMacs[0]->GetMacQueueScheduler()->BlockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                      AC_BE,
                                                      {WIFI_QOSDATA_QUEUE},
                                                      m_apMac->GetAddress(),
                                                      m_staMacs[0]->GetAddress(),
                                                      {0},
                                                      links);
}

void
EmlsrLinkSwitchTest::CheckResults()
{
    NS_TEST_ASSERT_MSG_NE(m_txPsdusPos, 0, "BA agreement establishment not completed");

    // Expected frame exchanges after ML setup and EML OMN exchange:
    //  1. (DL) ICF + CTS + ADDBA_REQ + ACK
    //  2. (UL) ADDBA_RESP + ACK
    //  3. (DL) ICF + CTS + DATA + BA
    //  4. (DL) ICF + CTS + DATA + BA
    //  5. (DL) ICF + CTS + DATA + BA
    //  6. (DL) ICF + CTS + DATA + BA
    //  7. (UL) ADDBA_REQ + ACK
    //  8. (DL) ICF + CTS + ADDBA_RESP + ACK
    //  9. (UL) DATA + BA
    // 10. (UL) RTS - CTS timeout
    // 11. (UL) (RTS + CTS + ) DATA + BA

    // frame exchange 11 is protected if SwitchAuxPhy is false or (SwitchAuxPhy is true and) the
    // main PHY switch can be interrupted
    bool fe11protected = !m_switchAuxPhy || m_resetCamStateAndInterruptSwitch;

    NS_TEST_EXPECT_MSG_EQ(m_countIcfFrames, 6, "Unexpected number of ICFs sent");

    // frame exchanges without RTS because the EMLSR client sent the initial frame through main PHY
    const std::size_t nFrameExchNoRts = fe11protected ? 3 : 4;

    const std::size_t nFrameExchWithRts = fe11protected ? 1 : 0;

    NS_TEST_ASSERT_MSG_GT_OR_EQ(m_txPsdus.size(),
                                m_txPsdusPos +
                                    m_countIcfFrames * 4 + /* frames in frame exchange with ICF */
                                    nFrameExchNoRts * 2 + /* frames in frame exchange without RTS */
                                    nFrameExchWithRts * 4 + /* frames in frame exchange with RTS */
                                    1,                      /* corrupted RTS */
                                "Insufficient number of TX PSDUs");

    // m_txPsdusPos points to the first ICF
    auto psduIt = std::next(m_txPsdus.cbegin(), m_txPsdusPos);

    const std::size_t nFrameExchanges =
        m_countIcfFrames + nFrameExchNoRts + nFrameExchWithRts + 1 /* corrupted RTS */;

    for (std::size_t i = 1; i <= nFrameExchanges; ++i)
    {
        if (i == 1 || (i >= 3 && i <= 6) || i == 8 || i == 10 || (i == 11 && fe11protected))
        {
            // frame exchanges with protection
            NS_TEST_EXPECT_MSG_EQ((psduIt->psduMap.size() == 1 &&
                                   (i < 9 ? psduIt->psduMap.at(SU_STA_ID)->GetHeader(0).IsTrigger()
                                          : psduIt->psduMap.at(SU_STA_ID)->GetHeader(0).IsRts())),
                                  true,
                                  "Expected a Trigger Frame (ICF)");
            psduIt++;
            if (i == 10)
            {
                continue; // corrupted RTS
            }
            NS_TEST_EXPECT_MSG_EQ((psduIt->psduMap.size() == 1 &&
                                   psduIt->psduMap.at(SU_STA_ID)->GetHeader(0).IsCts()),
                                  true,
                                  "Expected a CTS");
            psduIt++;
        }

        if (i == 1 || i == 2 || i == 7 || i == 8) // frame exchanges with ADDBA REQ/RESP frames
        {
            NS_TEST_EXPECT_MSG_EQ((psduIt->psduMap.size() == 1 &&
                                   psduIt->psduMap.at(SU_STA_ID)->GetHeader(0).IsMgt()),
                                  true,
                                  "Expected a management frame");
            psduIt++;
            NS_TEST_EXPECT_MSG_EQ((psduIt->psduMap.size() == 1 &&
                                   psduIt->psduMap.at(SU_STA_ID)->GetHeader(0).IsAck()),
                                  true,
                                  "Expected a Normal Ack");
        }
        else
        {
            NS_TEST_EXPECT_MSG_EQ((psduIt->psduMap.size() == 1 &&
                                   psduIt->psduMap.at(SU_STA_ID)->GetHeader(0).IsQosData()),
                                  true,
                                  "Expected a QoS Data frame");
            psduIt++;
            NS_TEST_EXPECT_MSG_EQ((psduIt->psduMap.size() == 1 &&
                                   psduIt->psduMap.at(SU_STA_ID)->GetHeader(0).IsBlockAck()),
                                  true,
                                  "Expected a BlockAck");
        }
        psduIt++;
    }
}

EmlsrCcaBusyTest::EmlsrCcaBusyTest(MHz_u auxPhyMaxChWidth)
    : EmlsrOperationsTestBase(std::string("Check EMLSR link switching (auxPhyMaxChWidth=") +
                              std::to_string(auxPhyMaxChWidth) + "MHz )"),
      m_auxPhyMaxChWidth(auxPhyMaxChWidth),
      m_channelSwitchDelay(MicroSeconds(75)),
      m_currMainPhyLinkId(0),
      m_nextMainPhyLinkId(0)
{
    m_nEmlsrStations = 1;
    m_nNonEmlsrStations = 1;
    m_linksToEnableEmlsrOn = {0, 1, 2}; // enable EMLSR on all links right after association
    m_mainPhyId = 1;
    m_establishBaUl = true;
    m_duration = Seconds(1);
    m_transitionDelay = {MicroSeconds(128)};
}

void
EmlsrCcaBusyTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    Simulator::Destroy();
}

void
EmlsrCcaBusyTest::DoSetup()
{
    Config::SetDefault("ns3::DefaultEmlsrManager::SwitchAuxPhy", BooleanValue(true));
    Config::SetDefault("ns3::EmlsrManager::AuxPhyChannelWidth", UintegerValue(m_auxPhyMaxChWidth));
    Config::SetDefault("ns3::EmlsrManager::AuxPhyMaxModClass", StringValue("EHT"));
    Config::SetDefault("ns3::WifiPhy::ChannelSwitchDelay", TimeValue(m_channelSwitchDelay));

    EmlsrOperationsTestBase::DoSetup();

    // use channels of different widths
    for (auto mac : std::initializer_list<Ptr<WifiMac>>{m_apMac, m_staMacs[0], m_staMacs[1]})
    {
        mac->GetWifiPhy(0)->SetOperatingChannel(
            WifiPhy::ChannelTuple{4, 40, WIFI_PHY_BAND_2_4GHZ, 0});
        mac->GetWifiPhy(1)->SetOperatingChannel(
            WifiPhy::ChannelTuple{58, 80, WIFI_PHY_BAND_5GHZ, 0});
        mac->GetWifiPhy(2)->SetOperatingChannel(
            WifiPhy::ChannelTuple{79, 160, WIFI_PHY_BAND_6GHZ, 0});
    }
}

void
EmlsrCcaBusyTest::TransmitPacketToAp(uint8_t linkId)
{
    m_staMacs[1]->GetDevice()->GetNode()->AddApplication(GetApplication(UPLINK, 1, 1, 2000));

    // force the transmission of the packet to happen now on the given link.
    // Multiple ScheduleNow calls are needed because Node::AddApplication() schedules a call to
    // Application::Initialize(), which schedules a call to Application::StartApplication(), which
    // schedules a call to PacketSocketClient::Send(), which finally generates the packet
    Simulator::ScheduleNow([=, this]() {
        Simulator::ScheduleNow([=, this]() {
            Simulator::ScheduleNow([=, this]() {
                m_staMacs[1]->GetFrameExchangeManager(linkId)->StartTransmission(
                    m_staMacs[1]->GetQosTxop(AC_BE),
                    m_staMacs[1]->GetWifiPhy(linkId)->GetChannelWidth());
            });
        });
    });

    // check that the other MLD started transmitting on the correct link
    Simulator::Schedule(TimeStep(1), [=, this]() {
        NS_TEST_EXPECT_MSG_EQ(m_staMacs[1]->GetWifiPhy(linkId)->IsStateTx(),
                              true,
                              "At time " << Simulator::Now().As(Time::NS)
                                         << ", other MLD did not start transmitting on link "
                                         << +linkId);
    });
}

void
EmlsrCcaBusyTest::StartTraffic()
{
    auto currMainPhyLinkId = m_staMacs[0]->GetLinkForPhy(m_mainPhyId);
    NS_TEST_ASSERT_MSG_EQ(currMainPhyLinkId.has_value(),
                          true,
                          "Main PHY is not operating on any link");
    m_currMainPhyLinkId = *currMainPhyLinkId;
    m_nextMainPhyLinkId = (m_currMainPhyLinkId + 1) % 2;

    // request the main PHY to switch to another link
    m_staMacs[0]->GetEmlsrManager()->SwitchMainPhy(
        m_nextMainPhyLinkId,
        false,
        EmlsrManager::DONT_RESET_BACKOFF,
        EmlsrManager::DONT_REQUEST_ACCESS,
        EmlsrDlTxopIcfReceivedByAuxPhyTrace{}); // trace info not used

    // the other MLD transmits a packet to the AP
    TransmitPacketToAp(m_nextMainPhyLinkId);

    // schedule another packet transmission slightly (10 us) before the end of aux PHY switch
    Simulator::Schedule(m_channelSwitchDelay - MicroSeconds(10),
                        &EmlsrCcaBusyTest::TransmitPacketToAp,
                        this,
                        m_currMainPhyLinkId);

    // first checkpoint is after that the preamble of the PPDU has been received
    Simulator::Schedule(MicroSeconds(8), &EmlsrCcaBusyTest::CheckPoint1, this);
}

/**
 *                               ┌───────────────┐
 *  [link X]                     │  other to AP  │CP3
 * ──────────────────────────────┴───────────────┴──────────────────────────────────────────────
 *  |------ main PHY ------|                  |------------------- aux PHY ---------------------
 *                         .\_              _/
 *                         .  \_          _/
 *                         .    \_      _/
 *                         .      \_  _/
 *  [link Y]               . CP1    \/ CP2
 *                         .┌───────────────┐
 *                         .│  other to AP  │
 * ─────────────────────────┴───────────────┴────────────────────────────────────────────────────
 *  |------------ aux PHY ----------|---------------------- main PHY ----------------------------
 *
 */

void
EmlsrCcaBusyTest::CheckPoint1()
{
    // first checkpoint is after that the preamble of the first PPDU has been received
    auto mainPhy = m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId);

    // 1. Main PHY is switching
    NS_TEST_EXPECT_MSG_EQ(mainPhy->IsStateSwitching(), true, "Main PHY is not switching");

    auto auxPhy = m_staMacs[0]->GetWifiPhy(m_nextMainPhyLinkId);
    NS_TEST_EXPECT_MSG_NE(mainPhy, auxPhy, "Main PHY is operating on an unexpected link");

    // 2. Aux PHY is receiving the PHY header
    NS_TEST_EXPECT_MSG_EQ(auxPhy->GetInfoIfRxingPhyHeader().has_value(),
                          true,
                          "Aux PHY is not receiving a PHY header");

    // 3. Main PHY dropped the preamble because it is switching
    NS_TEST_EXPECT_MSG_EQ(mainPhy->GetInfoIfRxingPhyHeader().has_value(),
                          false,
                          "Main PHY is receiving a PHY header");

    // 4. Channel access manager on destination link (Y) has been notified of CCA busy, but not
    // until the end of transmission (main PHY dropped the preamble and notified CCA busy until
    // end of transmission but the channel access manager on link Y does not yet have a listener
    // attached to the main PHY; aux PHY notified CCA busy until the end of the PHY header field
    // being received)
    const auto caManager = m_staMacs[0]->GetChannelAccessManager(m_nextMainPhyLinkId);
    const auto endTxTime = m_staMacs[1]->GetChannelAccessManager(m_nextMainPhyLinkId)->m_lastTxEnd;
    NS_TEST_ASSERT_MSG_EQ(caManager->m_lastBusyEnd.contains(WIFI_CHANLIST_PRIMARY),
                          true,
                          "No CCA information for primary20 channel");
    NS_TEST_EXPECT_MSG_GT_OR_EQ(
        caManager->m_lastBusyEnd[WIFI_CHANLIST_PRIMARY],
        Simulator::Now(),
        "ChannelAccessManager on destination link not notified of CCA busy");
    NS_TEST_EXPECT_MSG_LT(
        caManager->m_lastBusyEnd[WIFI_CHANLIST_PRIMARY],
        endTxTime,
        "ChannelAccessManager on destination link notified of CCA busy until end of transmission");

    // second checkpoint is after that the main PHY completed the link switch
    Simulator::Schedule(mainPhy->GetDelayUntilIdle() + TimeStep(1),
                        &EmlsrCcaBusyTest::CheckPoint2,
                        this);
}

void
EmlsrCcaBusyTest::CheckPoint2()
{
    // second checkpoint is after that the main PHY completed the link switch. The channel access
    // manager on destination link (Y) is expected to be notified by the main PHY that medium is
    // busy until the end of the ongoing transmission
    const auto caManager = m_staMacs[0]->GetChannelAccessManager(m_nextMainPhyLinkId);
    const auto endTxTime = m_staMacs[1]->GetChannelAccessManager(m_nextMainPhyLinkId)->m_lastTxEnd;
    NS_TEST_ASSERT_MSG_EQ(caManager->m_lastBusyEnd.contains(WIFI_CHANLIST_PRIMARY),
                          true,
                          "No CCA information for primary20 channel");
    NS_TEST_EXPECT_MSG_GT_OR_EQ(
        caManager->m_lastBusyEnd[WIFI_CHANLIST_PRIMARY],
        Simulator::Now(),
        "ChannelAccessManager on destination link not notified of CCA busy");
    NS_TEST_EXPECT_MSG_GT_OR_EQ(caManager->m_lastBusyEnd[WIFI_CHANLIST_PRIMARY],
                                endTxTime,
                                "ChannelAccessManager on destination link not notified of CCA busy "
                                "until end of transmission");

    // third checkpoint is after that the aux PHY completed the link switch
    Simulator::Schedule(m_channelSwitchDelay, &EmlsrCcaBusyTest::CheckPoint3, this);
}

void
EmlsrCcaBusyTest::CheckPoint3()
{
    // third checkpoint is after that the aux PHY completed the link switch. The channel access
    // manager on source link (X) is expected to be notified by the aux PHY that medium is
    // busy until the end of the ongoing transmission (even if the aux PHY was not listening to
    // link X when transmission started, its interface on link X recorded the transmission)
    const auto caManager = m_staMacs[0]->GetChannelAccessManager(m_currMainPhyLinkId);
    const auto endTxTime = m_staMacs[1]->GetChannelAccessManager(m_currMainPhyLinkId)->m_lastTxEnd;
    NS_TEST_ASSERT_MSG_EQ(caManager->m_lastBusyEnd.contains(WIFI_CHANLIST_PRIMARY),
                          true,
                          "No CCA information for primary20 channel");
    NS_TEST_EXPECT_MSG_GT_OR_EQ(caManager->m_lastBusyEnd[WIFI_CHANLIST_PRIMARY],
                                Simulator::Now(),
                                "ChannelAccessManager on source link not notified of CCA busy");
    NS_TEST_EXPECT_MSG_GT_OR_EQ(caManager->m_lastBusyEnd[WIFI_CHANLIST_PRIMARY],
                                endTxTime,
                                "ChannelAccessManager on source link not notified of CCA busy "
                                "until end of transmission");
}

WifiEmlsrTestSuite::WifiEmlsrTestSuite()
    : TestSuite("wifi-emlsr", Type::UNIT)
{
    AddTestCase(new EmlOperatingModeNotificationTest(), TestCase::Duration::QUICK);
    AddTestCase(new EmlOmnExchangeTest({1, 2}, MicroSeconds(0)), TestCase::Duration::QUICK);
    AddTestCase(new EmlOmnExchangeTest({1, 2}, MicroSeconds(2048)), TestCase::Duration::QUICK);
    AddTestCase(new EmlOmnExchangeTest({0, 1, 2, 3}, MicroSeconds(0)), TestCase::Duration::QUICK);
    AddTestCase(new EmlOmnExchangeTest({0, 1, 2, 3}, MicroSeconds(2048)),
                TestCase::Duration::QUICK);
    for (const auto& emlsrLinks :
         {std::set<uint8_t>{0, 1, 2}, std::set<uint8_t>{1, 2}, std::set<uint8_t>{0, 1}})
    {
        AddTestCase(new EmlsrDlTxopTest({1,
                                         0,
                                         emlsrLinks,
                                         {MicroSeconds(32)},
                                         {MicroSeconds(32)},
                                         MicroSeconds(512),
                                         true /* putAuxPhyToSleep */}),
                    TestCase::Duration::QUICK);
        AddTestCase(new EmlsrDlTxopTest({1,
                                         1,
                                         emlsrLinks,
                                         {MicroSeconds(64)},
                                         {MicroSeconds(64)},
                                         MicroSeconds(512),
                                         false /* putAuxPhyToSleep */}),
                    TestCase::Duration::QUICK);
        AddTestCase(new EmlsrDlTxopTest({2,
                                         2,
                                         emlsrLinks,
                                         {MicroSeconds(128), MicroSeconds(256)},
                                         {MicroSeconds(128), MicroSeconds(256)},
                                         MicroSeconds(512),
                                         true /* putAuxPhyToSleep */}),
                    TestCase::Duration::QUICK);
    }

    for (auto genBackoffAndUseAuxPhyCca : {true, false})
    {
        for (auto nSlotsLeft : std::initializer_list<uint8_t>{0, 4})
        {
            AddTestCase(new EmlsrUlTxopTest({{0, 1, 2},
                                             MHz_u{40},
                                             MHz_u{20},
                                             MicroSeconds(5504),
                                             3,
                                             genBackoffAndUseAuxPhyCca,
                                             nSlotsLeft,
                                             true, /* putAuxPhyToSleep */
                                             false /* switchMainPhyBackDelayTimeout */}),
                        TestCase::Duration::QUICK);
            AddTestCase(new EmlsrUlTxopTest({{0, 1},
                                             MHz_u{40},
                                             MHz_u{20},
                                             MicroSeconds(5504),
                                             1,
                                             genBackoffAndUseAuxPhyCca,
                                             nSlotsLeft,
                                             false, /* putAuxPhyToSleep */
                                             true /* switchMainPhyBackDelayTimeout */}),
                        TestCase::Duration::QUICK);
        }
    }

    for (bool switchAuxPhy : {true, false})
    {
        for (bool resetCamStateAndInterruptSwitch : {true, false})
        {
            for (auto auxPhyMaxChWidth : {MHz_u{20}, MHz_u{40}, MHz_u{80}, MHz_u{160}})
            {
                AddTestCase(new EmlsrLinkSwitchTest(
                                {switchAuxPhy, resetCamStateAndInterruptSwitch, auxPhyMaxChWidth}),
                            TestCase::Duration::QUICK);
            }
        }
    }

    AddTestCase(new EmlsrUlOfdmaTest(false), TestCase::Duration::QUICK);
    AddTestCase(new EmlsrUlOfdmaTest(true), TestCase::Duration::QUICK);

    AddTestCase(new EmlsrCcaBusyTest(MHz_u{20}), TestCase::Duration::QUICK);
    AddTestCase(new EmlsrCcaBusyTest(MHz_u{80}), TestCase::Duration::QUICK);
}

static WifiEmlsrTestSuite g_wifiEmlsrTestSuite; ///< the test suite
