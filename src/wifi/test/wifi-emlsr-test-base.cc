/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-emlsr-test-base.h"

#include "ns3/advanced-ap-emlsr-manager.h"
#include "ns3/advanced-emlsr-manager.h"
#include "ns3/attribute-container.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/ctrl-headers.h"
#include "ns3/eht-configuration.h"
#include "ns3/eht-frame-exchange-manager.h"
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/mgt-action-headers.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/node-list.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/pointer.h"
#include "ns3/qos-txop.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/rr-multi-user-scheduler.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/string.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-spectrum-phy-interface.h"

#include <algorithm>
#include <functional>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiEmlsrTest");

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
            const auto mustStartMsd = staMac->GetEmlsrManager()->GetInDeviceInterference() &&
                                      txDuration > MEDIUM_SYNC_THRESHOLD;

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
                        // and is expected to be running when check is performed (in 2 timesteps)
                        const auto msdTimer =
                            staMac->GetEmlsrManager()->GetElapsedMediumSyncDelayTimer(id);
                        const auto msdDuration =
                            staMac->GetEhtConfiguration()->m_mediumSyncDuration;
                        const auto msdWasRunning =
                            (msdTimer.has_value() && (msdDuration - *msdTimer > TimeStep(2)));
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
    NS_TEST_ASSERT_MSG_EQ((traceInfoIt != m_traceInfo.cend()),
                          true,
                          "Expected stored trace info: " << reason);
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
    RngSeedManager::SetSeed(m_rngSeed);
    RngSeedManager::SetRun(m_rngRun);
    int64_t streamNumber = m_streamNo;

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

    for (std::size_t id = 0; id < m_channelsStr.size(); ++id)
    {
        phyHelper.Set(id, "ChannelSettings", StringValue(m_channelsStr[id]));
        phyHelper.AddChannel(CreateObject<MultiModelSpectrumChannel>(), m_freqRanges[id]);
    }

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
    m_apMac = DynamicCast<ApWifiMac>(DynamicCast<WifiNetDevice>(apDevice.Get(0))->GetMac());

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

    if (m_nPhysPerEmlsrDevice < 3)
    {
        phyHelper = SpectrumWifiPhyHelper(m_nPhysPerEmlsrDevice);
        phyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        phyHelper.SetPcapCaptureType(WifiPhyHelper::PcapCaptureType::PCAP_PER_LINK);

        for (std::size_t id = 0; id < m_nPhysPerEmlsrDevice; ++id)
        {
            phyHelper.Set(id, "ChannelSettings", StringValue(m_channelsStr[id]));
            auto channel =
                DynamicCast<MultiModelSpectrumChannel>(m_apMac->GetWifiPhy(id)->GetChannel());
            NS_TEST_ASSERT_MSG_NE(channel,
                                  nullptr,
                                  "Channel " << +id << " is not a spectrum channel");
            phyHelper.AddChannel(channel, m_freqRanges[id]);
        }
    }

    NetDeviceContainer staDevices = wifi.Install(phyHelper, mac, wifiStaNodes);

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

    m_startAid = m_apMac->GetNextAssociationId();

    // schedule ML setup for one station at a time
    m_apMac->TraceConnectWithoutContext(
        "AssociatedSta",
        MakeCallback(&EmlsrOperationsTestBase::StaAssociated, this));
    for (const auto& [aci, ac] : wifiAcList)
    {
        m_apMac->GetQosTxop(aci)->TraceConnectWithoutContext(
            "BaEstablished",
            MakeCallback(&EmlsrOperationsTestBase::BaEstablishedDl, this));
        for (std::size_t id = 0; id < m_nEmlsrStations + m_nNonEmlsrStations; ++id)
        {
            m_staMacs[id]->GetQosTxop(aci)->TraceConnectWithoutContext(
                "BaEstablished",
                MakeCallback(&EmlsrOperationsTestBase::BaEstablishedUl, this).Bind(id));
        }
    }
    Simulator::Schedule(Seconds(0), [&]() { m_staMacs[0]->SetSsid(Ssid("ns-3-ssid")); });
}

Ptr<PacketSocketClient>
EmlsrOperationsTestBase::GetApplication(TrafficDirection dir,
                                        std::size_t staId,
                                        std::size_t count,
                                        std::size_t pktSize,
                                        uint8_t priority) const
{
    auto client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(pktSize));
    client->SetAttribute("MaxPackets", UintegerValue(count));
    client->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
    client->SetAttribute("Priority", UintegerValue(priority));
    client->SetRemote(dir == DOWNLINK ? m_dlSockets.at(staId) : m_ulSockets.at(staId));
    client->SetStartTime(Seconds(0)); // now
    client->SetStopTime(m_duration - Simulator::Now());

    return client;
}

void
EmlsrOperationsTestBase::StaAssociated(uint16_t aid, Mac48Address /* addr */)
{
    if (m_lastAid == aid)
    {
        // another STA of this non-AP MLD has already fired this callback
        return;
    }
    m_lastAid = aid;

    // wait some time (10ms) to allow the completion of association
    const auto delay = MilliSeconds(10);

    if (!m_establishBaDl.empty())
    {
        // trigger establishment of BA agreement with AP as originator
        Simulator::Schedule(delay, [=, this]() {
            m_apMac->GetDevice()->GetNode()->AddApplication(
                GetApplication(DOWNLINK, aid - m_startAid, 4, 1000, m_establishBaDl.front()));
        });
    }
    else if (!m_establishBaUl.empty())
    {
        // trigger establishment of BA agreement with AP as recipient
        Simulator::Schedule(delay, [=, this]() {
            m_staMacs[aid - m_startAid]->GetDevice()->GetNode()->AddApplication(
                GetApplication(UPLINK, aid - m_startAid, 4, 1000, m_establishBaUl.front()));
        });
    }
    else
    {
        Simulator::Schedule(delay, [=, this]() { SetSsid(aid - m_startAid + 1); });
    }
}

void
EmlsrOperationsTestBase::BaEstablishedDl(Mac48Address recipient,
                                         uint8_t tid,
                                         std::optional<Mac48Address> /* gcrGroup */)
{
    // wait some time (10ms) to allow the exchange of the data frame that triggered the Block Ack
    const auto delay = MilliSeconds(10);

    auto linkId = m_apMac->IsAssociated(recipient);
    NS_TEST_ASSERT_MSG_EQ(linkId.has_value(), true, "No link for association of " << recipient);
    auto aid = m_apMac->GetWifiRemoteStationManager(*linkId)->GetAssociationId(recipient);

    if (auto it = std::find(m_establishBaDl.cbegin(), m_establishBaDl.cend(), tid);
        it != m_establishBaDl.cend() && std::next(it) != m_establishBaDl.cend())
    {
        // trigger establishment of BA agreement with AP as originator
        Simulator::Schedule(delay, [=, this]() {
            m_apMac->GetDevice()->GetNode()->AddApplication(
                GetApplication(DOWNLINK, aid - m_startAid, 4, 1000, *std::next(it)));
        });
    }
    else if (!m_establishBaUl.empty())
    {
        // trigger establishment of BA agreement with AP as recipient
        Simulator::Schedule(delay, [=, this]() {
            m_staMacs[aid - m_startAid]->GetDevice()->GetNode()->AddApplication(
                GetApplication(UPLINK, aid - m_startAid, 4, 1000, m_establishBaUl.front()));
        });
    }
    else
    {
        Simulator::Schedule(delay, [=, this]() { SetSsid(aid - m_startAid + 1); });
    }
}

void
EmlsrOperationsTestBase::BaEstablishedUl(std::size_t index,
                                         Mac48Address recipient,
                                         uint8_t tid,
                                         std::optional<Mac48Address> /* gcrGroup */)
{
    // wait some time (10ms) to allow the exchange of the data frame that triggered the Block Ack
    const auto delay = MilliSeconds(10);

    if (auto it = std::find(m_establishBaUl.cbegin(), m_establishBaUl.cend(), tid);
        it != m_establishBaUl.cend() && std::next(it) != m_establishBaUl.cend())
    {
        // trigger establishment of BA agreement with AP as recipient
        Simulator::Schedule(delay, [=, this]() {
            m_staMacs[index]->GetDevice()->GetNode()->AddApplication(
                GetApplication(UPLINK, index, 4, 1000, *std::next(it)));
        });
    }
    else
    {
        Simulator::Schedule(delay, [=, this]() { SetSsid(index + 1); });
    }
}

void
EmlsrOperationsTestBase::SetSsid(std::size_t count)
{
    if (count < m_nEmlsrStations + m_nNonEmlsrStations)
    {
        // make the next STA start ML discovery & setup
        m_staMacs[count]->SetSsid(Ssid("ns-3-ssid"));
        return;
    }
    // all stations associated; start traffic if needed
    StartTraffic();
    // stop generation of beacon frames in order to avoid interference
    m_apMac->SetAttribute("BeaconGeneration", BooleanValue(false));
    // Set the short slot time on the 2.4 GHz link because it is not updated automatically given
    // that no more Beacon frames are sent
    for (std::size_t id = 0; id < m_nEmlsrStations + m_nNonEmlsrStations; ++id)
    {
        m_staMacs[id]->GetDevice()->GetPhy(0)->SetSlot(MicroSeconds(9));
    }
    // disconnect callbacks
    m_apMac->TraceDisconnectWithoutContext(
        "AssociatedSta",
        MakeCallback(&EmlsrOperationsTestBase::StaAssociated, this));
    for (const auto& [aci, ac] : wifiAcList)
    {
        m_apMac->GetQosTxop(aci)->TraceDisconnectWithoutContext(
            "BaEstablished",
            MakeCallback(&EmlsrOperationsTestBase::BaEstablishedDl, this));
        for (std::size_t id = 0; id < m_nEmlsrStations + m_nNonEmlsrStations; ++id)
        {
            m_staMacs[id]->GetQosTxop(aci)->TraceDisconnectWithoutContext(
                "BaEstablished",
                MakeCallback(&EmlsrOperationsTestBase::BaEstablishedUl, this).Bind(id));
        }
    }
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
    WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WifiRcvAddr::UNICAST, dest, 0);
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
